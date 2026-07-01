#include "mod/ClientInstanceHooks.h"

#include <optional>

#include "mod/ReconnectManager.h"

#include "ll/api/memory/Hook.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/network/DisconnectionScreenParams.h"
#include "mc/network/GameConnectionInfo.h"
#include "mc/network/connection/DisconnectFailReason.h"

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
    FlagDisconnectionHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$flagDisconnectionAndNotify,
    void,
    ::Connection::DisconnectFailReason disconnectReason
) {
    ReconnectManager::getInstance().onDisconnect(*this, disconnectReason);
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
    ReconnectManager::getInstance().onDisconnect(*this, std::nullopt);
    origin(params);
}

using FastReconnectHooks =
    ll::memory::HookRegistrar<SetGameConnectionInfoHook, FlagDisconnectionHook, FlagDisconnectionWithParamsHook>;

void registerHooks() { FastReconnectHooks::hook(); }

void unregisterHooks() { FastReconnectHooks::unhook(); }

} // namespace fast_reconnect
