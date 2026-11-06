# Architecture

Lanterne is a VST3 audio-effect that is a console for visualizer ROMs. The
plugin is the hardware, each folder under `ROMs/` is a cartridge, and the
audio passes through unchanged. This document describes the tree as it is
after the 0.1.0 migration: names, responsibilities, the bridge, and the
build.

---

## 1. Classes

| Class | Files | Role |
|---|---|---|
| `LanterneProcessor` | `Source/PluginProcessor.h`, `.cpp` | `juce::AudioProcessor`. Stereo in, stereo out, no processing. Owns `AudioAnalyzer`, fills `latestSnapshot` every block under a `juce::SpinLock`, exposes `getSnapshot()` to the editor. `getStateInformation` / `setStateInformation` stay empty. |
| `LanterneEditor` | `Source/PluginEditor.h`, `.cpp` | `juce::AudioProcessorEditor` plus a `juce::Timer`. Roughly 800×640: a 40 px ROM bar (previous button, next button, name label) over a `juce::WebBrowserComponent` fixed to the WebView2 backend. Builds the JSON payload each tick, injects metadata and settings, handles the right-click menu, and owns the ROM selector and hot-reload wiring. |
| `AudioAnalyzer` | `Source/AudioAnalyzer.h`, `.cpp` | Ring-buffer STFT, double-buffered output, band averages. See section 4. Class name unchanged by design. |
| `PluginSettings` | `Source/PluginSettings.h`, `.cpp` | POD holding the nine user-facing settings, their JSON persistence, and the helpers `bloomStrength()` and `frameCapToTimerMs()`. Also owns `getPluginBinaryDirectory()`, the single place that resolves where the plugin binary lives. |
| `SettingsWindow` | declared in `Source/PluginEditor.h` | Self-owned `juce::DocumentWindow` hosting `SettingsPanel`. Always on top, deleted through `MessageManager::callAsync`, never a child of the editor. |
| `SettingsPanel` | `Source/SettingsPanel.h`, `.cpp` | The settings UI itself: dark panel, cyan accents, one row per setting, apply and cancel buttons. |
| `ROMRegistry` | `Source/ROMRegistry.h`, `.cpp` | Scans the ROMs root for subfolders holding a valid `rom.json` and `index.html` with a non-empty name, produces `ROMManifest` records sorted by name. |
| `ROMWatcher` | `Source/ROMWatcher.h`, `.cpp` | 500 ms timer that snapshots paths and mtimes of the active ROM folder and fires a callback on any change. The editor turns that into a reload on the message thread. Polling is deliberate; there is no inotify. |

No plugin parameters exist. There is no preset host, no sidechain, no
automation, and no state to save into the host.

---

## 2. Data flow

```
processBlock (audio thread)
    → AudioAnalyzer::process(): mono mix, ring buffer, STFT, bands
    → snapshot copied under juce::SpinLock
editor timer (message thread, frame-cap interval)
    → buildJSON(snapshot) → webBrowser.evaluateJavascript(
          "window.__ROMReceiveAudio(" + json + ")")
    → once per ROM load: window.__ROMMeta = {...}
    → once per ROM load: window.__ROMSettings = {...} then
          window.__ROMOnSettings(window.__ROMSettings)
ROM
    → draws into its own canvas, reads nothing else from the host
```

The bridge is one-way. The ROM answers nothing. There is no Web Audio and
no analyser node inside the ROM.

### Frame rate

`PluginSettings::frameCapToTimerMs()` maps the cap to the timer interval:
30 fps → 33 ms, 60 fps → 16 ms, 120 fps → 8 ms, uncapped → 4 ms. The
payload size is also a setting: `bands` sends no array, `fft64` and
`fft256` send a binned average of the 256 analysis bins.

---

## 3. Settings

Nine settings in three groups:

| Group | Setting | Values | Default |
|---|---|---|---|
| Display | Resolution | 800×600, 1024×768, 1280×960 | 800×600 |
| Display | Pixel Ratio | 1×, 2× | 1× |
| Rendering | Antialiasing | Off, SMAA, MSAA 4× | SMAA |
| Rendering | Bloom | Off, Low, Medium, High | Medium |
| Rendering | Particle Count | 0.5×, 1×, 2× | 1× |
| Engine | Frame Rate Cap | 30, 60, 120, Uncapped | 60 |
| Engine | JSON Payload | Bands, FFT-64, FFT-256 | FFT-256 |
| Engine | Show FPS | Off, On | Off |

Persistence: JSON in `Lanterne.settings`, written beside the plugin binary.
A settings file left by the pre-Lanterne build in the same folder is read
once and rewritten under the new name; the fallback is in
`Source/PluginSettings.cpp`, and the history is in `docs/MIGRATION.md`.

### Why the settings window is a separate native window

`juce::WebBrowserComponent` on Windows hosts WebView2, and that is a native
Win32 child window. It sits above every JUCE-painted component no matter
what the JUCE z-order says. A settings panel added with `addAndMakeVisible()`
is created and painted correctly and stays invisible underneath it. The fix
is to give the panel its own OS window: `SettingsWindow` derives from
`juce::DocumentWindow`, is added to the desktop, is always on top, and
deletes itself through `MessageManager::callAsync`. Parenting it to the
editor instead would put it back under the browser view.

---

## 4. Audio analysis

`AudioAnalyzer` was rewritten before the migration, and its rules still
hold:

- No allocation on the audio thread. `prepare()` sizes every buffer;
  `process()` only writes into them.
- Ring buffer of 512 samples with a hop of 256, so the transform fires every
  256 samples regardless of the host block size.
- Hann window, 50 % overlap, `juce::dsp::FFT` of order 9.
- 256 output bins, magnitudes normalised from dB into 0–1 with a floor of
  −80 dB and a ceiling of 0 dB.
- Bands by frequency, not by index: bass 60–250 Hz, mid 250–4000 Hz,
  treble 4000–16000 Hz, converted with the current sample rate.
- Front/back buffers swapped under a spin lock, plus a second lock for RMS
  and bands.
- RMS is smoothed and clamped to 0–1. BPM is stubbed and always returns 0.

---

## 5. ROMs

Discovery: `ROMRegistry::scan()` looks at immediate subfolders of the ROMs
root, keeps the ones with a readable `rom.json`, an `index.html`, and a
non-empty `name`, and sorts them by name. The editor loads index 0 at
startup and moves with the arrow buttons.

Reload: `ROMWatcher` polls the active folder every 500 ms and calls back on
any path or mtime change. The editor reloads that ROM on the message thread.

Shipped cartridges:

- `ROMs/default/` — WebGL bars, an idle wave when the signal is silent, and
  a HUD. ES5 only, no CDN, no dependencies.
- `ROMs/cynicviz/` — nebula and particles on a local copy of three.js r161,
  with EffectComposer, UnrealBloomPass, and SMAA inlined into `main.js`.
  Nothing is fetched at runtime.

The bridge, the ROM manifest format, and the settings object are specified
in `docs/ROM_API.md`. Writing a cartridge is described in
`docs/ROM_AUTHORS.md`.

---

## 6. Resolution of ROMs and settings

Two locations, in this order:

1. `ROMs/` beside the plugin binary, which is where the post-build copy
   places it inside the VST3 bundle.
2. The `ROMs/` folder of this source tree, used when the plugin runs without
   a bundle copy.

The binary directory comes from `PluginSettings::getPluginBinaryDirectory()`,
which resolves the module that contains the plugin code rather than the host
executable. The same directory holds `Lanterne.settings`.

---

## 7. Build

CMake 3.22 or newer, C++17, `project(Lanterne VERSION 0.1.0 LANGUAGES C CXX)`.
Both languages are required: JUCE 8.0.12 compiles
`juce_graphics_Sheenbidi.c`, and a top-level `LANGUAGES CXX` alone left the
configure step without `CMAKE_C_COMPILE_OBJECT`.

JUCE is read from `.cache/JUCE` or from `LANTERNE_JUCE_DIR`. The VST3 SDK,
LV2 SDK, SheenBidi, and HarfBuzz all come from that pin; nothing is vendored
a second time. `NEEDS_WEB_BROWSER TRUE` and, when the WebView2 package is
found on Windows, `JUCE_USE_WIN_WEBVIEW2=1`, exactly as in the migrated
tree.

The only supported output is the VST3. See `BUILDING.md`.
