# Migration

What this tree came from, what was taken, and what was left behind.

---

## Source zip

| Item | Value |
|---|---|
| Path | `/run/media/deck/New Volume/flux canva/CMAKE_FLUXCANVASdesktop.zip` |
| Size | 1 623 899 629 bytes |
| SHA-256 | `c956d7a2a6741f918d3725c652543a10685a229172c6008912bbb3d595e0dde1` |

It was the only match for that name at the top level of the folder, so no
deeper search was needed. Six older zips sit in the same folder; none was
used.

Staging directory: `/run/media/deck/New Volume/flux canva/_stage_fluxcanvas/`,
used as a read-only source and deleted after verification. The zip itself was
never modified; the SHA-256 above was taken before unpacking and again after
the migration.

## JUCE pin from the zip

| Item | Value |
|---|---|
| Commit | `501c07674e1ad693085a7e7c398f205c2677f5da` |
| Described | `8.0.12-4-g501c076` |
| Upstream | https://github.com/juce-framework/JUCE |
| Where it lives now | `.cache/JUCE`, gitignored, with `origin` set to the upstream URL |

The zip carried JUCE as a submodule at that commit. The checkout was cloned
out of the staging tree, checked out at the same SHA, and verified clean
(`git status` empty, `git describe` printing `8.0.12-4-g501c076`). JUCE was
not upgraded, and no second copy exists: not vendored, not a submodule.

## What was read before anything was deleted

`CMakeLists.txt`, all of `Source/`, `ROMs/*/rom.json`, both `index.html`
files and both entry scripts, `build.ps1`, `.gitignore`, `.gitmodules`,
`docs/PROJECT.md`, `docs/CHANGES.md`, `docs/AUDIO_ENGINE_REWRITE.md`,
`docs/MILESTONE1.md`, `docs/MILESTONE2.md`, `CHANGELOG.md`, `README.md`,
the `VST3_SDK/LICENSE.txt` inside the pin, and the licence notices of every
file kept under `ROMs/cynicviz/lib/`.

## Excluded, and why

| Excluded | Why |
|---|---|
| `build/` (1.47 GB) | MSVC output: `.obj`, `.tlog`, `Release` artefacts, a prebuilt Windows VST3. Build output, not source. |
| `ROMs/cynicviz/lib/three.js-r161/` (572 MB) | A full clone of three.js, most of it `examples/`, `manual/`, `docs/`, `editor/`. Nothing loads it: `index.html` references only `lib/three.global.js` and `main.js`, `main.js` has no imports, and no path in either file points at the clone. The postprocessing code exists twice, inlined in `main.js` and as the small copies in `lib/`. |
| `chisiamoshader.txt` | Unrelated CSS for a page called DOMUS. Not part of the plugin. |
| `.git/` (676 MB) | Placeholder author `VisualROM <user@local>`, no remote. Lanterne starts a new local history. |
| `.gitmodules` | JUCE is a `.cache/JUCE` checkout in this tree, not a submodule. Keeping both layouts was explicitly avoided. |
| `docs/MILESTONE1.md`, `docs/MILESTONE2.md`, `docs/PROJECT.md`, `docs/CHANGES.md`, `docs/AUDIO_ENGINE_REWRITE.md` | Folded into `docs/ARCHITECTURE.md`, then dropped so one architecture document is current. `CHANGELOG.md` was rewritten for 0.1.0 rather than carrying the old history. |
| Editor swap files, `Thumbs.db`, `.DS_Store` | None were present in the zip. |

Kept on purpose: every file under `Source/`, both ROM folders without the
clone, `build.ps1` (moved to `scripts/`), and the CMake entry point.

## Names

| Was | Is |
|---|---|
| VisualROM, FluxCanvas, Flux Canva, CMAKE_FLUXCANVAS | Lanterne |
| `project(... LANGUAGES CXX)` | `project(Lanterne VERSION 0.1.0 LANGUAGES C CXX)` |
| CMake target `VisualROM` | `Lanterne` |
| `PRODUCT_NAME "VisualROM"` | `"Lanterne"` |
| `COMPANY_NAME VisualROM` | `MontroneDSP` |
| bundle id `com.VisualROM.VisualROM` | `com.MontroneDSP.Lanterne` |
| `PLUGIN_MANUFACTURER_CODE VROM` | `MnTr` |
| `PLUGIN_CODE Vr01` | `Lntr` |
| `VisualROMProcessor` | `LanterneProcessor` |
| `VisualROMEditor` | `LanterneEditor` |
| settings file `VisualROM.settings` | `Lanterne.settings`, with a one-time read of the old file |
| `VISUALROM_*` / `visualrom_*` CMake identifiers | `LANTERNE_*` / `lanterne_*` |
| HUD and error strings in `ROMs/default/main.js` | `Lanterne` |
| `build.ps1` at the root | `scripts/build.ps1`, project root derived from the new location |
| git author `VisualROM <user@local>` | not used; no such identity appears in this history |

Unchanged on purpose: `AudioAnalyzer`, `ROMRegistry`, `ROMWatcher`,
`PluginSettings`, `SettingsPanel`, `SettingsWindow`, `ROMs`, `rom.json`,
`__ROMReceiveAudio`, `__ROMMeta`, `__ROMSettings`, `__ROMOnSettings`, and
the filenames `PluginProcessor.*` / `PluginEditor.*`.

## Behaviour changes in this migration

1. **CMake C language.** `LANGUAGES C CXX`. Under JUCE 8.0.12 the old
   top-level `LANGUAGES CXX` left `CMAKE_C_COMPILE_OBJECT` undefined at
   generate time, because JUCE compiles `juce_graphics_Sheenbidi.c` and the
   C language was only enabled inside the JUCE subdirectory.
2. **ROM resolution.** Primary: `ROMs/` beside the plugin binary, resolved
   from the module that contains the plugin code rather than from the host
   executable. Fallback: the source tree `ROMs/`. The compile-time TODO
   pragma was removed with this change.
3. **Settings location.** `Lanterne.settings` beside the plugin binary, using
   the same directory helper, with a one-time read of `VisualROM.settings`.

No change to the audio path, the ROMs, the bridge, or the settings model.

## Configure status of this tree

The migration was made on a Linux machine without MSVC, without the Visual
Studio generator, and without WebView2. CMake configure was not run against
the Windows presets here, and no build was produced. The exact command to
run on a Windows machine is in `BUILDING.md`.

A local CMake configure of this tree on that Linux machine was run once as a
lists-file sanity check (C language fix, JUCE at `.cache/JUCE`) and completed
successfully: `cmake -B build -S . -G Ninja` wrote its build files. No compile
step followed, because the GTK, WebKitGTK, and OpenGL development packages a
Linux JUCE build needs are not installed here. That configure is not a
supported host build and is not evidence of a working plugin.
