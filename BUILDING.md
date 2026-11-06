# Building Lanterne

Windows VST3 is the supported build. The source tree may live on Linux, and
Linux is a place the source can sit, not a supported plugin host: the only
browser backend is WebView2, which is Windows.

## Requirements

- CMake 3.22 or newer
- A C++17 compiler; the supported toolchain is Visual Studio 2022
- JUCE 8.0.12 at commit `501c07674e1ad693085a7e7c398f205c2677f5da`
- WebView2 for the browser view that renders ROMs

## JUCE

The pin lives at `.cache/JUCE`, which is gitignored. Clone it and check out
the exact commit the tree was migrated against:

```bash
git clone https://github.com/juce-framework/JUCE.git .cache/JUCE
git -C .cache/JUCE checkout 501c07674e1ad693085a7e7c398f205c2677f5da
```

`git -C .cache/JUCE describe --tags` prints `8.0.12-4-g501c076` at that
commit.

To keep the checkout somewhere else, pass it to CMake, or set the
environment variable `LANTERNE_JUCE_DIR` when using `scripts/build.ps1`:

```powershell
$env:LANTERNE_JUCE_DIR = "D:\src\JUCE"
```

Do not commit JUCE. Do not add it as a submodule as well; `.cache/JUCE`
is the only supported layout in this tree.

## Configure and build (Windows)

The generator the project uses is Visual Studio 17 2022, x64. The exact
configure command is:

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Lanterne_VST3
```

Or use the script, which keeps the same commands as before the migration:

```powershell
.\scripts\build.ps1              # build Release (default)
.\scripts\build.ps1 rebuild      # clean + configure + build
.\scripts\build.ps1 build -Config Debug
.\scripts\build.ps1 clean        # delete build/
.\scripts\build.ps1 reconfigure  # configure only
.\scripts\build.ps1 open         # open Lanterne.sln
.\scripts\build.ps1 help
```

Presets are also available: `win-release` and `win-dev`.

The VST3 lands in:

```
build/Lanterne_artefacts/Release/VST3/Lanterne.vst3
```

### WebView2

The ROM view uses WebView2. CMake looks for the WebView2 package on Windows
and defines `JUCE_USE_WIN_WEBVIEW2=1` when it finds it. If a ROM shows
"getContext not available", install the package and reconfigure:

```powershell
Register-PackageSource -Provider NuGet -Name nugetRepository -Location https://www.nuget.org/api/v2
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.3485.44 -Source nugetRepository
```

Or set `JUCE_WEBVIEW2_PACKAGE_LOCATION` to the package directory and
reconfigure.

## Where the ROMs are read from

Two locations, in this order, and no others:

1. **Bundle copy.** The build copies `ROMs/` next to the plugin binary, into
   `Lanterne.vst3/Contents/<arch>/ROMs`. At runtime the plugin resolves the
   directory of its own DLL and reads `ROMs` from there. This is the path a
   normal install uses.
2. **Source tree.** If the bundle copy is absent, the plugin falls back to
   the `ROMs/` folder next to `Source/` in this repository. This is the
   developer path while working from a build folder.

If neither exists, the ROM bar reads `No ROMs found`.

Settings are written as `Lanterne.settings` beside the plugin binary. If a
settings file from the pre-Lanterne build sits in the same folder, it is
read once and rewritten under the new name.

## Configure status of this tree

`0.1.0` is a local migration on a Linux machine without MSVC or WebView2.

- A CMake configure of this tree was run locally once as a lists-file sanity
  check (`cmake -B build -S . -G Ninja`, JUCE taken from `.cache/JUCE`) and
  completed successfully. It proves the lists file and the `LANGUAGES C CXX`
  fix are sound; the compile step was not run, because the GTK, WebKitGTK,
  and OpenGL development packages a Linux JUCE build needs are absent, and
  Linux is not a supported host for this release.
- The Windows presets were not configured on that machine. Configure and
  build the `win-release` preset on Windows with Visual Studio 2022. The
  full record is in `docs/MIGRATION.md`.
