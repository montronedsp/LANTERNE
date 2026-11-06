# Third-party notices

Every dependency listed here has files either in this tree or inside the
documented JUCE pin at `.cache/JUCE` (see `BUILDING.md`). The pin is not
committed; `LANTERNE_JUCE_DIR` may point at a checkout elsewhere.

---

## JUCE

- URL: https://github.com/juce-framework/JUCE
- Pin: commit `501c07674e1ad693085a7e7c398f205c2677f5da` (`8.0.12-4-g501c076`)
- License: JUCE 8 licence, **AGPLv3 option** (see `LICENSE.md` in the pin)
- Where: `.cache/JUCE`, documented in `BUILDING.md`, not committed

Lanterne is distributed under AGPL-3.0-or-later, which is the option of the
JUCE licence this tree is built against.

## VST3 SDK (Steinberg)

- URL: https://github.com/steinbergmedia/vst3sdk
- Pin: shipped inside the JUCE pin
- License: **MIT**, Copyright (c) 2025, Steinberg Media Technologies GmbH,
  as stated in `modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt`
  of the pin
- Where: inside the JUCE pin

## LV2 SDK (lv2, lilv, serd, sord, sratom)

- URL: https://github.com/lv2/lv2 and the related projects
- Pin: shipped inside the JUCE pin
- License: ISC-style terms, as stated in the `COPYING` files under
  `modules/juce_audio_processors_headless/format_types/LV2_SDK/` of the pin
- Where: inside the JUCE pin

## SheenBidi

- URL: https://github.com/Tehreer/SheenBidi
- Pin: shipped inside the JUCE pin
- License: Apache-2.0, see `modules/juce_graphics/unicode/sheenbidi/LICENSE` in the pin
- Where: inside the JUCE pin, compiled by JUCE as `juce_graphics_Sheenbidi.c`

## HarfBuzz

- URL: https://github.com/harfbuzz/harfbuzz
- Pin: shipped inside the JUCE pin
- License: "Old MIT", see `modules/juce_graphics/fonts/harfbuzz/COPYING` in the pin
- Where: inside the JUCE pin, compiled by JUCE as `juce_graphics_Harfbuzz.cpp`

## Other code inside the JUCE pin

zlib, libpng, and the other libraries JUCE vendors carry their own notices in
their directories inside the pin. They are not relicensed by this project.
Open the pin and read the notice next to the sources.

## three.js r161

- URL: https://github.com/mrdoob/three.js tag `v0.161.0`
- Version: r161 (`REVISION = '161'`)
- License: MIT, Copyright 2010-2023 Three.js Authors
- Where, in this tree:
  - `ROMs/cynicviz/lib/three.global.js` — local UMD build loaded by `index.html`
  - `ROMs/cynicviz/lib/three.module.min.js` — local module build referenced by the
    postprocessing files
  - `ROMs/cynicviz/lib/` — EffectComposer, RenderPass, UnrealBloomPass, ShaderPass,
    MaskPass, Pass, CopyShader, LuminosityHighPassShader, SMAAPass, SMAAShader
  - the same postprocessing sources are inlined into `ROMs/cynicviz/main.js`
- Notice: each kept file carries an `@license` header with
  `SPDX-License-Identifier: MIT`

## SMAA

- URL: https://github.com/iryoku/smaa
- Version: v2.8
- License: MIT with the upstream attribution clarification, see the upstream README
- Where: the SMAA shaders and pass ship inside
  `ROMs/cynicviz/lib/SMAAPass.js`, `ROMs/cynicviz/lib/SMAAShader.js`, and the
  inlined copy inside `ROMs/cynicviz/main.js`. Their upstream notice is kept.

---

ROMs are separate works. See `ROMs/default/LICENSE`, `ROMs/cynicviz/LICENSE`,
and `docs/ROM_AUTHORS.md`.
