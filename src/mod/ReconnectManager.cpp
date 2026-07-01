#include "mod/ReconnectManager.h"

#include "mod/FastReconnectMod.h"

#include "magic_enum/magic_enum.hpp"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/gui/screens/ScreenController.h"
#include "mc/client/gui/screens/ScreenEvent.h"
#include "mc/client/gui/screens/ScreenEventType.h"
#include "mc/client/gui/controls/UIPropertyBag.h"
#include "mc/client/gui/screens/controllers/DisconnectScreenController.h"
#include "mc/deps/core/string/StringHash.h"
#include "mc/deps/input/enums/ButtonState.h"
#include "mc/network/ConnectionType.h"

namespace fast_reconnect {

namespace {

ll::io::Logger& logger() { return FastReconnectMod::getInstance().getSelf().getLogger(); }

constexpr std::size_t kReadinessChecks = 40;

} // namespace

ReconnectManager& ReconnectManager::getInstance() {
    static ReconnectManager instance;
    return instance;
}

void ReconnectManager::setConfig(Config const& config) { mConfig = config; }

void ReconnectManager::onConnectionInfoSet(::Social::GameConnectionInfo const& info) { mLastConnection = info; }

void ReconnectManager::onStartJoin(bool isJoiningLocalServer, std::string const& serverName) {
    if (!isJoiningLocalServer && !serverName.empty()) {
        mServerName = serverName;
    }
    mHandlingDisconnect = false;
    mReconnectActive    = false;
    mLeaveRequested     = false;
}

void ReconnectManager::onJoinedLevel() { mAttempts = 0; }

void ReconnectManager::onDisconnect(
    ::IClientInstance&                                client,
    std::optional<::Connection::DisconnectFailReason> reason,
    std::optional<::Connection::DisconnectionStage>   stage,
    std::string const&                                serverMessage
) {
    mClient = &client;
    if (!mConfig.enabled || mHandlingDisconnect) {
        return;
    }
    if (!mLastConnection) {
        mLastConnection = client.getGameConnectionInfo();
    }
    if (!isReconnectableConnection()) {
        logger().info("Not reconnecting: no reconnectable server connection");
        return;
    }
    if (!shouldReconnect(reason)) {
        logger().info("Not reconnecting: reason '{}' is not recoverable", magic_enum::enum_name(*reason));
        return;
    }
    if (mAttempts >= mConfig.maxAttempts) {
        logger().warn("Giving up: {} reconnect attempts failed in a row", mAttempts);
        return;
    }
    mHandlingDisconnect = true;
    mReconnectActive    = true;
    mLeaveRequested     = false;
    mManualRequest      = false;
    ++mAttempts;
    logger().info(
        "Disconnected from '{}' (reason: {}, message: '{}'), reconnecting in {:.1f}s (attempt {}/{})",
        mServerName,
        reason ? magic_enum::enum_name(*reason) : "unknown",
        serverMessage,
        mConfig.delaySeconds,
        mAttempts,
        mConfig.maxAttempts
    );
    (void)stage;
    scheduleAttempt();
}

void ReconnectManager::onDisconnectScreenOpened(::DisconnectScreenController& controller) {
    mDisconnectScreen = &controller;
    if (mBoundScreen != &controller) {
        mBoundScreen = &controller;
        static_cast<::ScreenController&>(controller).bindBool(
            ::StringHash("#fastreconnect_marker"),
            [] {
                ReconnectManager::getInstance().markResourcePackDetected();
                return false;
            },
            [] { return true; }
        );
    }
}

void ReconnectManager::onDisconnectScreenClosed(::DisconnectScreenController& controller) {
    if (mDisconnectScreen == &controller) {
        mDisconnectScreen = nullptr;
    }
    if (mBoundScreen == &controller) {
        mBoundScreen = nullptr;
    }
}

void ReconnectManager::onClientUpdate(::IClientInstance& client) {
    if (!mNextActionAt) {
        return;
    }
    mClient = &client;
    if (std::chrono::steady_clock::now() < *mNextActionAt) {
        return;
    }
    mNextActionAt.reset();
    if (mRpDetected && !mManualRequest) {
        logger().info("Auto reconnect suppressed, waiting for the Reconnect button");
        mReconnectActive    = false;
        mHandlingDisconnect = false;
        return;
    }
    tryConnect(mChecksLeft);
}

void ReconnectManager::onScreenButtonEvent(::ScreenController& controller, ::ScreenEvent& event) {
    if (mDisconnectScreen == nullptr || static_cast<::ScreenController*>(mDisconnectScreen) != &controller) {
        return;
    }
    if (event.type != ::ScreenEventType::ButtonEvent) {
        return;
    }
    auto const& button = *event.data->button;
    if (static_cast<::ButtonState>(button.state) != ::ButtonState::Up) {
        return;
    }
    ::UIPropertyBag* properties = button.properties;
    if (properties == nullptr || !properties->has("#fastreconnect")) {
        return;
    }
    logger().info("Reconnect button pressed");
    forceReconnect();
}

void ReconnectManager::forceReconnect() {
    if (!mConfig.enabled || mClient == nullptr || !isReconnectableConnection()) {
        return;
    }
    mReconnectActive    = true;
    mHandlingDisconnect = true;
    mLeaveRequested     = false;
    mManualRequest      = true;
    mChecksLeft         = kReadinessChecks;
    mNextActionAt       = std::chrono::steady_clock::now();
}

void ReconnectManager::markResourcePackDetected() {
    if (!mRpDetected) {
        mRpDetected = true;
        logger().info("Resource pack detected, auto reconnect disabled in favor of the button");
    }
}

void ReconnectManager::reset() {
    mNextActionAt.reset();
    mClient           = nullptr;
    mDisconnectScreen = nullptr;
    mBoundScreen      = nullptr;
    mLastConnection.reset();
    mServerName.clear();
    mHandlingDisconnect = false;
    mReconnectActive    = false;
    mLeaveRequested     = false;
    mRpDetected         = false;
    mManualRequest      = false;
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

bool ReconnectManager::isReconnectableConnection() const {
    if (!mLastConnection) {
        return false;
    }
    switch (static_cast<::Social::ConnectionType>(mLastConnection->mType)) {
    case ::Social::ConnectionType::IPv4:
    case ::Social::ConnectionType::IPv6:
    case ::Social::ConnectionType::UnknownIP:
        return true;
    default:
        return false;
    }
}

void ReconnectManager::scheduleAttempt() {
    mChecksLeft   = kReadinessChecks;
    mNextActionAt = std::chrono::steady_clock::now()
                  + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                        std::chrono::duration<double>(mConfig.delaySeconds)
                  );
}

void ReconnectManager::tryConnect(std::size_t readinessChecksLeft) {
    if (!mConfig.enabled || !mReconnectActive || !mClient || !mLastConnection) {
        return;
    }
    auto const waitAndRetry = [this, readinessChecksLeft] {
        if (readinessChecksLeft == 0) {
            logger().warn("Client never became ready to reconnect, giving up (screen: {})", mClient->getScreenName());
            mHandlingDisconnect = false;
            mReconnectActive    = false;
            return;
        }
        mChecksLeft   = readinessChecksLeft - 1;
        mNextActionAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    };

    auto const screen = mClient->getScreenName();
    if (screen.find("disconnect") != std::string::npos) {
        if (!mLeaveRequested) {
            mLeaveRequested = true;
            logger().info("Dismissing disconnect screen");
            if (mDisconnectScreen != nullptr) {
                mDisconnectScreen->_processLeaveScreen();
            } else {
                mClient->setLeaveGameInProgressAsReadyToContinue();
            }
        }
        waitAndRetry();
        return;
    }
    if (!mClient->isLeaveGameDone() || !mClient->isReadyToReconnect()) {
        waitAndRetry();
        return;
    }
    mReconnectActive    = false;
    mHandlingDisconnect = false;
    auto const& host = static_cast<std::string const&>(mLastConnection->mHostIpAddress);
    int const   port = mLastConnection->mPort;
    logger().info("Reconnecting to '{}' ({}:{})", mServerName.empty() ? host : mServerName, host, port);
    mClient->connectToThirdPartyServer(host, port);
}

} // namespace fast_reconnect
