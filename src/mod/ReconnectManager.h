#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

#include "mod/Config.h"

#include "mc/network/GameConnectionInfo.h"
#include "mc/network/connection/DisconnectFailReason.h"
#include "mc/network/connection/DisconnectionStage.h"

class IClientInstance;
class DisconnectScreenController;
class ScreenController;
struct ScreenEvent;

namespace fast_reconnect {

class ReconnectManager {
public:
    static ReconnectManager& getInstance();

    void setConfig(Config const& config);

    void onConnectionInfoSet(::Social::GameConnectionInfo const& info);

    void onStartJoin(bool isJoiningLocalServer, std::string const& serverName);

    void onJoinedLevel();

    void onDisconnect(
        ::IClientInstance&                                client,
        std::optional<::Connection::DisconnectFailReason> reason,
        std::optional<::Connection::DisconnectionStage>   stage,
        std::string const&                                serverMessage
    );

    void onDisconnectScreenOpened(::DisconnectScreenController& controller);

    void onDisconnectScreenClosed(::DisconnectScreenController& controller);

    void onClientUpdate(::IClientInstance& client);

    void onScreenButtonEvent(::ScreenController& controller, ::ScreenEvent& event);

    void forceReconnect();

    void markResourcePackDetected();

    void reset();

private:
    ReconnectManager() = default;

    [[nodiscard]] bool shouldReconnect(std::optional<::Connection::DisconnectFailReason> reason) const;
    [[nodiscard]] bool isReconnectableConnection() const;

    void scheduleAttempt();
    void tryConnect(std::size_t readinessChecksLeft);

    Config                                               mConfig{};
    ::IClientInstance*                                   mClient           = nullptr;
    ::DisconnectScreenController*                        mDisconnectScreen = nullptr;
    ::DisconnectScreenController*                        mBoundScreen      = nullptr;
    std::optional<::Social::GameConnectionInfo>          mLastConnection;
    std::string                                          mServerName;
    bool                                                 mHandlingDisconnect = false;
    bool                                                 mReconnectActive    = false;
    bool                                                 mLeaveRequested     = false;
    bool                                                 mRpDetected         = false;
    bool                                                 mManualRequest      = false;
    int                                                  mAttempts           = 0;
    std::size_t                                          mChecksLeft         = 0;
    std::optional<std::chrono::steady_clock::time_point> mNextActionAt;
};

} // namespace fast_reconnect
