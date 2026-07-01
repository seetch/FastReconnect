# FastReconnect

A client-side [LeviLamina](https://github.com/LiteLDev/LeviLamina) mod for Minecraft Bedrock Edition that automatically reconnects you to the last server after a kick or connection loss.

Works with game instances launched via [LeviLauncher](https://github.com/LiteLDev/LeviLauncher).

The project consists of two parts:

| Part | Required | What it does |
| --- | --- | --- |
| **Mod** (`src/`) | Yes | Detects the disconnect, dismisses the disconnect screen and rejoins the server automatically |
| **Resource pack** (`resource_pack/`) | Optional | Adds a **Reconnect** button to the disconnect screen; when active, automatic reconnect is disabled and reconnecting happens only via the button |

## Features

- Automatically rejoins the last third-party server after a kick, timeout or network error
- Dismisses the disconnect screen and reconnects through the regular join flow
- Optional "Reconnect" button on the disconnect screen (via the resource pack); with the pack active the mod switches from automatic to button-driven reconnect
- Configurable delay and maximum number of attempts
- Skips unrecoverable disconnects (version mismatch, bans by platform restrictions, user-initiated leave)
- Attempt counter resets after a successful join

## Installation

### Mod

```
lip install github.com/seetch/FastReconnect
```

Or import the release `fast-reconnect-client-windows-x64.zip` into LeviLauncher manually.

### Resource pack (optional)

1. Download `FastReconnectUI.mcpack` from the releases page and open it with Minecraft, **or** copy the `resource_pack` folder into `.../Minecraft Bedrock/Users/Shared/games/com.mojang/resource_packs/FastReconnectUI` inside your game instance.
2. In the game go to **Settings → Global Resources** and activate **FastReconnect UI**.

The button only works when the mod is installed: the pack merely adds the button to the disconnect screen, while the mod handles the press.

## Configuration

`mods/fast-reconnect/config/config.json`:

```json
{
    "version": 2,
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
