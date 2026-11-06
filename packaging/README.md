# Packaging

There is no release. This note exists because the build copies files.

## The post-build copy

`CMakeLists.txt` ends with:

```cmake
add_custom_command(TARGET Lanterne_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/ROMs
        $<TARGET_FILE_DIR:Lanterne_VST3>/ROMs
    COMMENT "Copying ROMs next to plugin binary")
```

`$<TARGET_FILE_DIR:Lanterne_VST3>` is the folder holding the built plugin
file, so after a build the tree looks like:

```
build/Lanterne_artefacts/Release/VST3/Lanterne.vst3/
  Contents/
    x86_64-win/
      Lanterne.vst3      the plugin binary
      ROMs/              copy of this repository's ROMs/
        default/
        cynicviz/
```

At runtime the plugin takes the directory of its own binary and looks for
`ROMs` there first. That copy is what a user-facing install would ship.

## Shipping a folder

Copy `Lanterne.vst3` and keep `ROMs/` next to the binary inside it. Adding a
cartridge is adding a folder to `ROMs/`; removing one is deleting that
folder. Nothing else is registered anywhere.

## Not included

No installer, no Linux package, no curl script, no signing, no release
artefact. `packaging/` holds this page and stops here.
