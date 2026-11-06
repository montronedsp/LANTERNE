# Roadmap

Short and honest.

## Next

- Confirm the Windows VST3 build from this tree: configure, build, load in a
  host, both shipped ROMs on screen, settings window on top of the browser
  view.
- Push 0.1.0 to https://github.com/montronedsp/LANTERNE, which exists and is
  empty, and use `main` as the canonical branch. Nothing has been pushed yet.

## Later

- A Linux or macOS host. Not now: WebView2 is the only browser backend in
  this tree, and adding a second one is a port, not a patch.
- More cartridges under `ROMs/`.
- Documentation work that needs a running build to be truthful.

## Not planned

- No analyzer features, no measurement readouts, no metering, no loudness.
- No plugin parameters and no automation, unless a ROM needs one and the
  bridge changes with it.
- No ROM store, no payments, no licence enforcement inside the plugin.
- No AU, no Standalone, no CI badges until there is something to publish.
