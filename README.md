# FastReconnect

A client-side [LeviLamina](https://github.com/LiteLDev/LeviLamina) mod for Minecraft Bedrock Edition that automatically reconnects you to the last server after a kick or connection loss.

Works with game instances launched via [LeviLauncher](https://github.com/LiteLDev/LeviLauncher).

## Features

- Automatically rejoins the last third-party server after a kick, timeout or network error
- Configurable delay and maximum number of attempts
- Skips unrecoverable disconnects (version mismatch, bans by platform restrictions, user-initiated leave)
- Attempt counter resets after a successful join

## Installation

```
lip install github.com/seetch/FastReconnect
```

Or import the release `.zip` into LeviLauncher manually.

## Configuration

`mods/fast-reconnect/config/config.json`:

```json
{
    "version": 1,
    "enabled": true,
    "delaySeconds": 3.0,
    "maxAttempts": 5,
    "reconnectOnKick": true
}
```

| Option | Description |
| --- | --- |
| `enabled` | Master switch for the mod |
| `delaySeconds` | Delay before a reconnect attempt |
| `maxAttempts` | Max consecutive failed attempts before giving up |
| `reconnectOnKick` | Also reconnect when kicked (not only on connection errors) |

## Building

```
xmake f -y -p windows -a x64 -m release
xmake
```

The packed mod appears in `bin/fast-reconnect/`.

Requires the MSVC Build Tools and a `clang-cl` toolchain (LLVM).

## Notes

- The mod only reconnects to external (third-party) servers, not to local or friends' worlds.
- Some server networks disallow modified clients; use at your own risk.

## License

MIT
