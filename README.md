# Lanterne

**Lanterne** is an open-source visual instrument by MontroneDSP.
Lanterne is a console that runs visual ROMs. Audio passes through
unchanged. Pictures come out.

Lanterne is a visual instrument. It listens so a ROM can draw.
It is not a meter, and it is not a measurement tool.

## Features

### Console

The plugin is the hardware. A ROM bar sits above the plugin window with a
previous button, a next button, and the name and author of the loaded ROM.
Right-click anywhere in the plugin to open the settings window: resolution,
pixel ratio, antialiasing, bloom, particle count, frame rate cap, JSON payload
size, and an on-screen FPS readout. Settings are written beside the plugin
binary and injected into the ROM on the next frame.

Audio reaches the output exactly as it arrived. The host listens at a frame
rate you set and pushes one JSON object into the
loaded ROM. The ROM draws full-window in an embedded browser. Editing a file
inside the active ROM folder reloads that ROM within half a second; the folder
is polled, so nothing else has to be installed.

### Cartridges

A cartridge is a folder under `ROMs/`. It contains `rom.json`, `index.html`,
and the entry script the manifest names. The host scans that folder at
startup and lists every cartridge that has a manifest, an HTML file, and a
non-empty name.

- `default/` is the starter ROM: a WebGL bar view, an idle wave when there is
  no signal, and a HUD. ES5, no CDN, no dependencies.
- `cynicviz/` is the included visual ROM: nebula, particles, bloom driven by
  the low end, and SMAA, all on a local copy of three.js r161. Nothing is
  fetched from a CDN.
- `doom/` runs the shareware Doom episode as a WebAssembly engine
  (jacobenget/doom.wasm v0.1.0, GPL-2.0). The engine and the IWAD baked
  into it travel in the folder, nothing is fetched at runtime, and the game
  runs without sound. Provenance and controls are in `ROMs/doom/NOTICE.md`.

Another ROM is a folder. Copy one of the shipped folders, write your own
`rom.json`, `index.html`, and entry script, and it appears on the ROM bar.

### What it does not do

It does not process the audio. It has no plugin parameters and no
automation. There is no sidechain, no preset host, and no BPM detection;
the HUD shows `BPM: --`. It is not a meter, and it is not a measurement
tool.

## Quick start

Windows VST3. Load the plugin. Audio in, picture out. Use the arrow buttons
on the ROM bar to move between `Default`, `CynicViz`, and `Doom`, right-click
for settings, and drop new cartridges into `ROMs/`.

## Building

See [BUILDING.md](BUILDING.md). CMake 3.22+, C++17, Visual Studio 2022, the
pinned JUCE commit, and WebView2 for the browser view. The source tree may
live on Linux, and the files build there far enough to be useful for reading
and editing, but Linux is not a supported host for this plugin.

## License

Lanterne software is distributed under **AGPL-3.0-or-later**.
Example ROMs shipped in `ROMs/` are MIT. See `LICENSE`, `COPYRIGHT`,
`THIRD_PARTY_NOTICES.md`, and `docs/ROM_AUTHORS.md`.

## Source

This 0.1.0 tree is a local migration. The intended public repository is
https://github.com/montronedsp/LANTERNE
and it has not been pushed. `main` will be the canonical branch.

## Project information

- [Contributing](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Governance](GOVERNANCE.md)
- [Roadmap](ROADMAP.md)
- [Security](SECURITY.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Building](BUILDING.md)

Lanterne 0.1.0 — MontroneDSP
https://www.montronedsp.com
