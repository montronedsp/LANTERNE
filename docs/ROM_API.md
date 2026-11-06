# ROM API

Everything the host sends to a ROM, and nothing the host expects back. Field
names below are taken from the code in `Source/`, not from memory.

The bridge is a read-only runtime API. A ROM answers nothing: no return
value, no Web Audio, no analyser node.

---

## 1. `window.__ROMReceiveAudio(data)`

Called by the host on every timer tick, at the configured frame cap.

```js
window.__ROMReceiveAudio({
  rms: 0.0,              // number, 0.0 – 1.0, smoothed RMS of the input
  bpm: 0.0,              // number, always 0.0 — detection is not implemented
  fft: [ /* array */ ],  // optional, see below
  bands: {
    bass: 0.0,           // number, 0.0 – 1.0, 60 – 250 Hz average
    mid: 0.0,            // number, 0.0 – 1.0, 250 – 4000 Hz average
    treble: 0.0          // number, 0.0 – 1.0, 4000 – 16000 Hz average
  }
});
```

`fft` is present unless the payload setting is `bands`:

| Payload setting | `fft` in the JSON |
|---|---|
| `bands` | absent |
| `fft64` | 64 numbers, each the average of 4 of the 256 analysis bins |
| `fft256` | 256 numbers, one per analysis bin |

Every number is normalised into 0.0 – 1.0: the magnitudes are mapped from
−80 dB … 0 dB, and the bands are averages of those magnitudes. The object is
built with `juce::DynamicObject` and serialised with `juce::JSON`, then
passed as the single argument of the call:

```js
window.__ROMReceiveAudio({...})
```

A ROM should install the function before the first tick:

```js
window.__ROMReceiveAudio = function (data) { audioData = data; };
```

## 2. `window.__ROMMeta`

Injected once after a ROM is loaded, immediately after the first
`__ROMReceiveAudio` tick.

```js
window.__ROMMeta = {
  name: "CynicViz",     // from rom.json, string
  author: "Cynic Games",// from rom.json, string
  version: "1.0",       // from rom.json, string
  api: "1"              // from rom.json, string
};
```

Quoted as JavaScript string literals with `'` and `\` escaped.

## 3. `window.__ROMSettings`

Injected once after a ROM is loaded, in the same tick as `__ROMMeta`, and
again whenever settings are applied.

```js
window.__ROMSettings = {
  pixelRatio: 1.0,        // number: 1.0 or 2.0
  antialias: 'smaa',      // 'off' | 'smaa' | 'msaa4x'
  bloom: 'medium',        // 'off' | 'low' | 'medium' | 'high'
  bloomStrength: 0.44,    // number: 0.00, 0.25, 0.44, 0.70
  particleScale: 1.0,     // number: 0.5, 1.0, 2.0
  showFPS: false          // boolean
};
```

Resolution, frame rate cap, and payload size are host-side settings and are
not part of this object.

## 4. `window.__ROMOnSettings`

A function the ROM may define. The host appends this to the settings
injection:

```js
if (typeof window.__ROMOnSettings === 'function')
  window.__ROMOnSettings(window.__ROMSettings);
```

Define it to apply settings without reloading the ROM.

---

## What the ROM sends back

Nothing. The host never reads a value from the ROM.

---

## Manifest: `rom.json`

```json
{
  "name": "CynicViz",
  "author": "Cynic Games",
  "version": "1.0",
  "entry": "main.js",
  "api": "1"
}
```

| Field | Required | Meaning |
|---|---|---|
| `name` | yes | Non-empty, otherwise the folder is skipped by the scan |
| `author` | no | Shown in the ROM bar next to the name |
| `version` | no | Passed through to `__ROMMeta` |
| `entry` | no | Script loaded with `index.html`; defaults to `main.js` |
| `api` | no | Passed through to `__ROMMeta` |

A cartridge folder must contain this file and an `index.html`. Everything
else in the folder is loaded by the HTML itself.

## Loading

The host opens `index.html` of the active cartridge with
`juce::WebBrowserComponent` on the WebView2 backend, as a file URL.
Relative paths inside the folder resolve normally, which is how
`lib/three.global.js` is loaded by the cynicviz ROM.

## Reload

Touching any file in the active ROM folder reloads that ROM within 500 ms.
After a reload, `__ROMMeta` and `__ROMSettings` are injected again.
