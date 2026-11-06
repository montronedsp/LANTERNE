# Writing a ROM

Short version: a ROM is a folder. The host gives it audio and a window. It
gives the host nothing.

## Adding a cartridge

1. Copy `ROMs/default/` to `ROMs/<yours>/`.
2. Edit `rom.json`: `name` must be non-empty, or the folder is skipped.
3. Put your drawing code in the script `rom.json` names as `entry`, loaded
   from `index.html`.
4. Restart the plugin, or touch the folder if the plugin is already running;
   it polls every 500 ms and reloads.

```
ROMs/yours/
  rom.json      name, author, version, entry, api
  index.html    your canvas, your markup
  main.js       your drawing code
```

That is the whole installer. There is no store, no payment code, and no
licence check in the host.

## The bridge, in one screen

The host calls into the page. The page never calls back.

```js
// 1. receive audio, once per frame at the configured frame cap
window.__ROMReceiveAudio = function (data) {
  // data.rms    0..1
  // data.bpm    always 0
  // data.fft    64 or 256 numbers, 0..1, absent when payload = 'bands'
  // data.bands  { bass, mid, treble } 0..1
};

// 2. injected once after load: name, author, version, api
//    window.__ROMMeta = { name, author, version, api }

// 3. injected once after load, and again when settings change
//    window.__ROMSettings = { pixelRatio, antialias, bloom,
//                             bloomStrength, particleScale, showFPS }

// 4. optional: called right after the settings object
window.__ROMOnSettings = function (settings) {
  // apply without reloading
};
```

No Web Audio. No analyser node. No return values. The full field list is in
`docs/ROM_API.md`.

## Who owns what

The host is AGPL-3.0-or-later. **That does not AGPL your ROM.** The bridge
is a read-only runtime API, and a ROM that speaks it is not a derivative of
the host. Your ROM stays your work under the licence you choose.

Paid or closed cartridges are allowed later and are not part of this
repository. The two example ROMs shipped here are MIT, see
`ROMs/default/LICENSE` and `ROMs/cynicviz/LICENSE`.
