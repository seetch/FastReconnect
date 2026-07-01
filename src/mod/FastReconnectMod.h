#pragma once

#include "mod/Config.h"

#include "ll/api/event/ListenerBase.h"
#include "ll/api/mod/NativeMod.h"

namespace fast_reconnect {

class FastReconnectMod {
public:
    static FastReconnectMod& getInstance();

    FastReconnectMod() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod& mSelf;

    Config mConfig{};

    ll::event::ListenerPtr mStartJoinListener;
    ll::event::ListenerPtr mJoinLevelListener;
};

} // namespace fast_reconnect
