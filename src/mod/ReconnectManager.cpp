#include "mod/ReconnectManager.h"

#include <chrono>
#include <utility>

#include "mod/FastReconnectMod.h"

#include "ll/api/thread/ClientThreadExecutor.h"

#include "magic_enum/magic_enum.hpp"

#include "mc/client/events/PlayerJoinWorldContext.h"
#include "mc/client/game/ClientInstance.h"

namespace fast_reconnect {

namespace {

ll::io::Logger& logger() { return FastReconnectMod::getInstance().getSelf().getLogger(); }

constexpr std::size_t kReadinessChecks = 20;

} // namespace

ReconnectManager& ReconnectManager::getInstance() {
    static ReconnectManager instance;
    return instance;
}

void ReconnectManager::setConfig(Config const& config) { mConfig = config; }

void ReconnectManager::onConnectionInfoSet(::Social::GameConnectionInfo const& info) { mLastConnection = info; }

void ReconnectManager::onStartJoin(bool isJoiningLocalServer, std::string const& serverName) {
    mLastJoinWasLocal = isJoiningLocalServer;
    if (!isJoiningLocalServer) {
        mServerName = serverName;
    }
    mHandlingDisconnect = false;
}

void ReconnectManager::onJoinedLevel() { mAttempts = 0; }

void ReconnectManager::onDisconnect(
    ::ClientInstance&                                 client,
    std::optional<::Connection::DisconnectFailReason> reason
) {
    if (!mConfig.enabled || mHandlingDisconnect) {
        return;
    }
    if (mLastJoinWasLocal || !mLastConnection) {
        return;
    }
    if (!shouldReconnect(reason)) {
        logger().info(
            "Not reconnecting: reason '{}' is not recoverable",
            reason ? magic_enum::enum_name(*reason) : "unknown"
        );
        return;
    }
    if (mAttempts >= mConfig.maxAttempts) {
        logger().warn("Giving up: {} reconnect attempts failed in a row", mAttempts);
        return;
    }
    mHandlingDisconnect = true;
    ++mAttempts;
    logger().info(
        "Disconnected from '{}' (reason: {}), reconnecting in {:.1f}s (attempt {}/{})",
        mServerName,
        reason ? magic_enum::enum_name(*reason) : "unknown",
        mConfig.delaySeconds,
        mAttempts,
        mConfig.maxAttempts
    );
    scheduleAttempt(client);
}

void ReconnectManager::reset() {
    if (mPendingTask) {
        mPendingTask->cancel();
        mPendingTask.reset();
    }
    mLastConnection.reset();
    mServerName.clear();
    mLastJoinWasLocal   = true;
    mHandlingDisconnect = false;
    mAttempts           = 0;
}

bool ReconnectManager::shouldReconnect(std::optional<::Connection::DisconnectFailReason> reason) const {
    if (!reason) {
        return true;
    }
    using ::Connection::DisconnectFailReason;
    switch (*reason) {
    case DisconnectFailReason::InternalUserLeaveGameAttempted:
    case DisconnectFailReason::InternalNoFailOccurred:
    case DisconnectFailReason::VersionMismatch:
    case DisconnectFailReason::EditionVersionMismatch:
    case DisconnectFailReason::EditionMismatch:
    case DisconnectFailReason::OutdatedServer:
    case DisconnectFailReason::OutdatedClient:
    case DisconnectFailReason::NotAllowed:
    case DisconnectFailReason::NotAuthenticated:
    case DisconnectFailReason::MultiplayerDisabled:
    case DisconnectFailReason::CrossPlatformDisabled:
    case DisconnectFailReason::NoPremiumPlatform:
        return false;
    case DisconnectFailReason::Kicked:
    case DisconnectFailReason::KickedForExploit:
    case DisconnectFailReason::KickedForIdle:
        return mConfig.reconnectOnKick;
    default:
        return true;
    }
}

void ReconnectManager::scheduleAttempt(::ClientInstance& client) {
    auto const delay = std::chrono::duration_cast<ll::coro::Duration>(
        std::chrono::duration<double>(mConfig.delaySeconds)
    );
    mPendingTask = ll::thread::ClientThreadExecutor::getDefault().executeAfter(
        [this, &client] { tryConnect(client, kReadinessChecks); },
        delay
    );
}

void ReconnectManager::tryConnect(::ClientInstance& client, std::size_t readinessChecksLeft) {
    if (!mConfig.enabled || !mLastConnection) {
        return;
    }
    if (!client.isReadyToReconnect()) {
        if (readinessChecksLeft == 0) {
            logger().warn("Client never became ready to reconnect, giving up");
            mHandlingDisconnect = false;
            return;
        }
        mPendingTask = ll::thread::ClientThreadExecutor::getDefault().executeAfter(
            [this, &client, readinessChecksLeft] { tryConnect(client, readinessChecksLeft - 1); },
            std::chrono::milliseconds(500)
        );
        return;
    }
    logger().info("Reconnecting to '{}' ({})", mServerName, mLastConnection->mHostIpAddress.get());
    ::PlayerJoinWorldContext context{std::string{}, false, false, false, true};
    client.startExternalNetworkWorld(*mLastConnection, mServerName, std::move(context));
}

} // namespace fast_reconnect
