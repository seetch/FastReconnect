#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "mod/Config.h"

#include "ll/api/data/CancellableCallback.h"

#include "mc/network/GameConnectionInfo.h"
#include "mc/network/connection/DisconnectFailReason.h"

class ClientInstance;

namespace fast_reconnect {

class ReconnectManager {
public:
    static ReconnectManager& getInstance();

    void setConfig(Config const& config);

    void onConnectionInfoSet(::Social::GameConnectionInfo const& info);

    void onStartJoin(bool isJoiningLocalServer, std::string const& serverName);

    void onJoinedLevel();

    void onDisconnect(::ClientInstance& client, std::optional<::Connection::DisconnectFailReason> reason);

    void reset();

private:
    ReconnectManager() = default;

    [[nodiscard]] bool shouldReconnect(std::optional<::Connection::DisconnectFailReason> reason) const;

    void scheduleAttempt(::ClientInstance& client);
    void tryConnect(::ClientInstance& client, std::size_t readinessChecksLeft);

    Config                                         mConfig{};
    std::optional<::Social::GameConnectionInfo>    mLastConnection;
    std::string                                    mServerName;
    bool                                           mLastJoinWasLocal   = true;
    bool                                           mHandlingDisconnect = false;
    int                                            mAttempts           = 0;
    std::shared_ptr<ll::data::CancellableCallback> mPendingTask;
};

} // namespace fast_reconnect
