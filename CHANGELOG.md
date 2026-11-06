# Changelog

All notable changes to Lanterne are recorded here.

---

## 0.1.0 — local migration

First entry. This tree is a migration of the local Windows project the 0.1.0
snapshot was taken from. Which zip it came from, and what was dropped, is
recorded in `docs/MIGRATION.md`.

### Added

- Project identity: `Lanterne`, `MontroneDSP`, bundle
  `com.MontroneDSP.Lanterne`, manufacturer code `MnTr`, plugin code `Lntr`,
  version 0.1.0, settings file `Lanterne.settings`.
- Licence set: `LICENSE` and `COPYING` (AGPL-3.0-or-later), `COPYRIGHT`,
  `THIRD_PARTY_NOTICES.md`, MIT licences for the two example ROMs.
- Documentation: `README.md`, `BUILDING.md`, `docs/ARCHITECTURE.md`,
  `docs/ROM_API.md`, `docs/ROM_AUTHORS.md`, `docs/MIGRATION.md`, and the
  project administration files at the root.
- `CMakePresets.json` with `win-release` and `win-dev`.

### Changed

- `project(... LANGUAGES C CXX)`. JUCE 8.0.12 compiles
  `juce_graphics_Sheenbidi.c`, and with only C enabled at the top level the
  configure step failed with `Missing variable: CMAKE_C_COMPILE_OBJECT`.
- JUCE is expected at `.cache/JUCE` at a fixed commit, overridable with
  `LANTERNE_JUCE_DIR`, instead of a committed checkout.
- ROM resolution now starts from the plugin binary directory (the copy the
  build places inside the VST3 bundle) and falls back to the source tree
  `ROMs/` folder. The previous compile-time TODO is gone.
- Settings are located through the plugin binary directory, with a one-time
  read of the legacy settings file written by the pre-Lanterne build.
- `build.ps1` moved to `scripts/` with its project root derived from the
  new location.

### Removed

- Committed `build/` output, the full three.js repository clone, the zip's
  `.git` history, and unrelated scratch files.

No behaviour of the ROM bridge, the ROMs, or the audio path changed in this
migration.
