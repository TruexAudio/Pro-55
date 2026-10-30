cmake_minimum_required(VERSION 3.24)

project(Pro55 VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PRO55_BUILD_TESTS "Build Pro-55 data-format tests" ON)
option(PRO55_ALT_GUI "Build the approved refined panel artwork" ON)

if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Set JUCE_DIR to a JUCE checkout containing CMakeLists.txt")
endif()

add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

set(CLAP_JUCE_EXTENSIONS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/clap-juce-extensions"
    CACHE PATH "Path to the clap-juce-extensions checkout")
if(EXISTS "${CLAP_JUCE_EXTENSIONS_DIR}/CMakeLists.txt")
    set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CLAP_JUCE_EXTENSIONS_DIR}"
                     "${CMAKE_BINARY_DIR}/clap-juce-extensions"
                     EXCLUDE_FROM_ALL)
else()
    message(WARNING "CLAP support disabled: clap-juce-extensions was not found")
endif()

set(PRO55_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PRO55_FORMATS AU)
endif()

juce_add_plugin(Pro55
    COMPANY_NAME "TrueX Audio"
    BUNDLE_ID "com.truexaudio.pro55"
    PLUGIN_MANUFACTURER_CODE Trux
    PLUGIN_CODE Pr55
    FORMATS ${PRO55_FORMATS}
    PRODUCT_NAME "Pro-55"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth)

if(COMMAND clap_juce_extensions_plugin)
    clap_juce_extensions_plugin(TARGET Pro55
        CLAP_ID "com.truexaudio.pro55"
        CLAP_FEATURES instrument synthesizer analog
        CLAP_MANUAL_URL "https://truexaudio.com/"
        CLAP_SUPPORT_URL "https://truexaudio.com/"
        CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 0
        CLAP_USE_JUCE_PARAMETER_RANGES ALL)
    # Used by the explicitly applied Windows CLAP coverage dependency patch.
    target_include_directories(Pro55_CLAP PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

if(PRO55_ALT_GUI)
    set(PRO55_BACKGROUND_ASSET "assets/panel.png")
    set(PRO55_KEYMAP_ASSET "assets/keyboard_keymap.png")
    set(PRO55_BLACK_BUTTON_ASSET "assets/button_black.png")
    set(PRO55_GREY_BUTTON_ASSET "assets/button_grey.png")
    set(PRO55_ORANGE_BUTTON_ASSET "assets/button_orange.png")
    set(PRO55_BLACK_KNOB_ASSET "assets/knob_black.png")
    set(PRO55_METAL_KNOB_ASSET "assets/knob_metal.png")
else()
    set(PRO55_BACKGROUND_ASSET "assets/panel_legacy.png")
    set(PRO55_KEYMAP_ASSET)
    set(PRO55_BLACK_BUTTON_ASSET "assets/button_black_legacy.png")
    set(PRO55_GREY_BUTTON_ASSET "assets/button_grey_legacy.png")
    set(PRO55_ORANGE_BUTTON_ASSET "assets/button_orange_legacy.png")
    set(PRO55_BLACK_KNOB_ASSET "assets/knob_black_legacy.png")
    set(PRO55_METAL_KNOB_ASSET "assets/knob_metal_legacy.png")
endif()

juce_add_binary_data(Pro55Assets
    SOURCES
        presets/Factory.pro55bank
        "${PRO55_BACKGROUND_ASSET}"
        ${PRO55_KEYMAP_ASSET}
        assets/about_logo.png
        assets/bpf_indicator.png
        "${PRO55_BLACK_BUTTON_ASSET}"
        "${PRO55_GREY_BUTTON_ASSET}"
        "${PRO55_ORANGE_BUTTON_ASSET}"
        assets/filter_selector.png
        "${PRO55_BLACK_KNOB_ASSET}"
        "${PRO55_METAL_KNOB_ASSET}"
        assets/led.png
        assets/midi_indicator.png
        assets/program_digits.png
        assets/voice_digits.png
        assets/wheel.png)

target_sources(Pro55 PRIVATE
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/dsp/Pro54Engine.cpp
    src/dsp/Pro54Engine.h
    src/presets/ProgramAddress.h
    src/presets/PresetLibrary.cpp
    src/presets/PresetLibrary.h
    src/presets/PresetTypes.cpp
    src/presets/PresetTypes.h)

target_compile_definitions(Pro55 PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    PRO55_ALT_GUI=$<BOOL:${PRO55_ALT_GUI}>
    PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(Pro55 PRIVATE /Zc:__cplusplus)
endif()

target_link_libraries(Pro55 PRIVATE
    Pro55Assets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(PRO55_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
