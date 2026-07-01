#pragma once

namespace fast_reconnect {

struct Config {
    int    version         = 2;
    bool   enabled         = true;
    double delaySeconds    = 3.0;
    int    maxAttempts     = 5;
    bool   reconnectOnKick = true;
};

} // namespace fast_reconnect
