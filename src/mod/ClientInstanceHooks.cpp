#include "mod/ClientInstanceHooks.h"

#include <optional>
#include <string>

#include "mod/ReconnectManager.h"

#include "ll/api/memory/Hook.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/ViewRequest.h"
#include "mc/client/gui/screens/ScreenController.h"
#include "mc/client/gui/screens/ScreenEvent.h"
#include "mc/client/gui/screens/controllers/DisconnectScreenController.h"
#include "mc/client/network/ClientNetworkHandler.h"
#include "mc/network/DisconnectionScreenParams.h"
#include "mc/network/GameConnectionInfo.h"
#include "mc/network/connection/DisconnectFailReason.h"
#include "mc/network/connection/DisconnectionStage.h"

namespace fast_reconnect {

LL_TYPE_INSTANCE_HOOK(
    SetGameConnectionInfoHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$setGameConnectionInfo,
    void,
    ::Social::GameConnectionInfo const& gameConnection
) {
    ReconnectManager::getInstance().onConnectionInfoSet(gameConnection);
    origin(gameConnection);
}

LL_TYPE_INSTANCE_HOOK(
    NetworkDisconnectHook,
    ll::memory::HookPriority::Normal,
    ClientNetworkHandler,
    &ClientNetworkHandler::$onDisconnect,
    void,
    ::NetworkIdentifier const&               source,
    ::Connection::DisconnectFailReason const discoReason,
    ::Connection::DisconnectionStage const   disconnectStage,
    ::std::string const&                     messageFromServer,
    ::std::string const&                     messageBodyOverride,
    bool                                     skipMessage,
    ::std::string const&                     telemetryOverride
) {
    ReconnectManager::getInstance().onDisconnect(mClient, discoReason, disconnectStage, messageFromServer);
    origin(
        source,
        discoReason,
        disconnectStage,
        messageFromServer,
        messageBodyOverride,
        skipMessage,
        telemetryOverride
    );
}

LL_TYPE_INSTANCE_HOOK(
    FlagDisconnectionHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$flagDisconnectionAndNotify,
    void,
    ::Connection::DisconnectFailReason disconnectReason
) {
    ReconnectManager::getInstance().onDisconnect(*this, disconnectReason, std::nullopt, {});
    origin(disconnectReason);
}

LL_TYPE_INSTANCE_HOOK(
    FlagDisconnectionWithParamsHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$flagDisconnectionAndNotifyWithParams,
    void,
    ::DisconnectionScreenParams const& params
) {
    ReconnectManager::getInstance().onDisconnect(*this, std::nullopt, std::nullopt, {});
    origin(params);
}

LL_TYPE_INSTANCE_HOOK(
    ClientUpdateHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$update,
    bool,
    bool isInitFinished
) {
    auto const result = origin(isInitFinished);
    ReconnectManager::getInstance().onClientUpdate(*this);
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    DisconnectScreenOpenHook,
    ll::memory::HookPriority::Normal,
    DisconnectScreenController,
    &DisconnectScreenController::$onOpen,
    void
) {
    ReconnectManager::getInstance().onDisconnectScreenOpened(*this);
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    ScreenButtonEventHook,
    ll::memory::HookPriority::Normal,
    ScreenController,
    &ScreenController::_handleButtonEvent,
    ::ui::ViewRequest,
    ::ScreenEvent& screenEvent
) {
    ReconnectManager::getInstance().onScreenButtonEvent(*this, screenEvent);
    return origin(screenEvent);
}

LL_TYPE_INSTANCE_HOOK(
    DisconnectScreenDtorHook,
    ll::memory::HookPriority::Normal,
    DisconnectScreenController,
    &DisconnectScreenController::$dtor,
    void
) {
    ReconnectManager::getInstance().onDisconnectScreenClosed(*this);
    origin();
}

using FastReconnectHooks = ll::memory::HookRegistrar<
    SetGameConnectionInfoHook,
    NetworkDisconnectHook,
    FlagDisconnectionHook,
    FlagDisconnectionWithParamsHook,
    ClientUpdateHook,
    DisconnectScreenOpenHook,
    DisconnectScreenDtorHook,
    ScreenButtonEventHook>;

void registerHooks() { FastReconnectHooks::hook(); }

void unregisterHooks() { FastReconnectHooks::unhook(); }

} // namespace fast_reconnect
