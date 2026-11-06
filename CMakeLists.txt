# Lanterne — VST3 visual ROM console (Milestone: 0.1.0)
# Requires CMake 3.22+ and a JUCE 8 checkout at .cache/JUCE (or LANTERNE_JUCE_DIR).

cmake_minimum_required(VERSION 3.22)

project(Lanterne VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

# JUCE location: .cache/JUCE by default, overridable with -DLANTERNE_JUCE_DIR=...
set(LANTERNE_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.cache/JUCE" CACHE PATH
    "Path to the pinned JUCE checkout (see BUILDING.md)")
if(NOT EXISTS "${LANTERNE_JUCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "JUCE not found at '${LANTERNE_JUCE_DIR}'. "
    "Clone it as described in BUILDING.md, or pass -DLANTERNE_JUCE_DIR=<path>.")
endif()

# WebView2 (Chromium) on Windows: enables Canvas/WebGL in the ROM. Optional; if not found, plugin uses legacy engine.
if(WIN32)
  list(APPEND CMAKE_MODULE_PATH "${LANTERNE_JUCE_DIR}/extras/Build/CMake")
  find_package(WebView2 QUIET)
  if(WebView2_FOUND)
    add_compile_definitions(JUCE_USE_WIN_WEBVIEW2=1)
    include_directories(${WebView2_INCLUDE_DIRS})
    set(LANTERNE_USE_WEBVIEW2 TRUE)
  else()
    message(STATUS "WebView2 not found: ROM will show 'getContext not available'. Install NuGet package Microsoft.Web.WebView2 to fix.")
  endif()
endif()

add_subdirectory("${LANTERNE_JUCE_DIR}" juce)

set(lanterne_plugin_extra "")
if(LANTERNE_USE_WEBVIEW2)
  list(APPEND lanterne_plugin_extra NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_plugin(Lanterne
    COMPANY_NAME MontroneDSP
    COMPANY_WEBSITE "https://www.montronedsp.com"
    COMPANY_COPYRIGHT "Copyright (C) 2026 MontroneDSP"
    BUNDLE_ID com.MontroneDSP.Lanterne
    PLUGIN_MANUFACTURER_CODE MnTr
    PLUGIN_CODE Lntr
    FORMATS VST3
    PRODUCT_NAME "Lanterne"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    ${lanterne_plugin_extra})

juce_generate_juce_header(Lanterne)

target_sources(Lanterne
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AudioAnalyzer.cpp
        Source/ROMRegistry.cpp
        Source/ROMWatcher.cpp
        Source/PluginSettings.cpp
        Source/SettingsPanel.cpp)

target_compile_definitions(Lanterne
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Lanterne
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Copy ROMs folder next to the built plugin binary so the runtime can find ROMs/default/
add_custom_command(TARGET Lanterne_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/ROMs
        $<TARGET_FILE_DIR:Lanterne_VST3>/ROMs
    COMMENT "Copying ROMs next to plugin binary")
