# Doom cartridge — provenance and licence

This cartridge runs the shareware Doom episode inside the plugin's webview.
Nothing is downloaded at runtime: the engine and the game data travel with
the ROM folder, and the page never issues a network request.

## What ships here

| File | Origin | Licence |
|---|---|---|
| `rom.json`, `index.html`, `main.js` | Written for this tree by MontroneDSP | MIT (`LICENSE`) |
| `engine/doom.wasm.b64.js` | Base64 of the release asset `doom-v0.1.0.wasm` from [jacobenget/doom.wasm](https://github.com/jacobenget/doom.wasm) tag `v0.1.0` | GPL-2.0 (`engine/LICENSE`) |
| `engine/LICENSE` | GNU GPL version 2 text | GPL-2.0 |

The shareware IWAD (`DOOM1.WAD`, Episode 1: *Knee-Deep in the Dead*) is
**compiled into the engine binary** by that project's build, so no separate
game-data file is present in this folder. id Software's shareware terms
permit redistributing that episode; all Doom content remains copyright id
Software / ZeniMax Media.

The engine binary also contains the Doom source port code, which id Software
released under GPL-2.0 in 1997.

## Verify the engine payload

```sh
tail -n 1 engine/doom.wasm.b64.js \
  | sed -e "s/^window.__DOOM_WASM_B64 = '//" -e "s/';$//" \
  | base64 -d | sha256sum
```

Expected SHA-256:

```
8edfe49a7583fd975199969302d8e9adcf8e714d0af72bf3e672f991fd810faa
```

Corresponding source for the engine: <https://github.com/jacobenget/doom.wasm>
at tag `v0.1.0` (its `Makefile` builds the module with the WASI SDK and embeds
the shareware WAD).

## Controls

| Action | Keys |
|---|---|
| Move forward / back | `W` `S` or `Up` `Down` |
| Strafe | `A` `D` or `,` `.` |
| Turn | `Q` `E` or `Left` `Right` |
| Fire | `F` or `Ctrl` |
| Use / open | `Space` |
| Run | `Shift` |
| Automap | `Tab` |
| Menu | `Esc`, navigate with arrows, confirm with `Enter` |

## Known limits

- **No sound.** This engine build exposes no audio import, so the game runs
  silent. The host's audio payload is still delivered to the page through
  `__ROMReceiveAudio`; the ROM has nothing to do with it.
- **Keyboard only.** The engine's import surface has no mouse entry point.
- **Episode 1 only**, unless you replace the engine payload with a build
  against your own IWAD.
- Saves are disabled (`gameSaving` imports return zero).

## Trademark

DOOM is a registered trademark of id Software / ZeniMax Media. This cartridge
is an unofficial, unendorsed technical demonstration of freely redistributable
software; it is not affiliated with or approved by id Software or ZeniMax.
