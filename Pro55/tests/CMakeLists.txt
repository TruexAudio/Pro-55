add_executable(Pro55FormatTests
    PresetLibraryTests.cpp
    ../src/presets/PresetLibrary.cpp
    ../src/presets/PresetTypes.cpp)

target_include_directories(Pro55FormatTests PRIVATE ../src)
target_link_libraries(Pro55FormatTests PRIVATE juce::juce_core)
target_compile_definitions(Pro55FormatTests PRIVATE JUCE_USE_CURL=0)

add_test(NAME Pro55FormatTests COMMAND Pro55FormatTests)

# Processor-level tests use isolated temporary banks, never the real user bank.
add_executable(Pro55ProgrammerTests ProgrammerTests.cpp)
target_include_directories(Pro55ProgrammerTests PRIVATE ../src)
target_link_libraries(Pro55ProgrammerTests PRIVATE Pro55 Pro55Assets juce::juce_audio_utils juce::juce_dsp)
target_compile_definitions(Pro55ProgrammerTests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
if(MSVC)
    target_compile_options(Pro55ProgrammerTests PRIVATE /Zc:__cplusplus)
endif()
add_test(NAME Pro55ProgrammerTests COMMAND Pro55ProgrammerTests)

add_executable(Pro55AudioSmokeTest
    AudioSmokeTest.cpp
    ../src/dsp/Pro54Engine.cpp)

target_include_directories(Pro55AudioSmokeTest PRIVATE ../src)
target_link_libraries(Pro55AudioSmokeTest PRIVATE juce::juce_audio_basics)
target_compile_definitions(Pro55AudioSmokeTest PRIVATE JUCE_USE_CURL=0)
if(MSVC)
    target_compile_options(Pro55AudioSmokeTest PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME Pro55AudioSmokeTest COMMAND Pro55AudioSmokeTest)

# Explicit diagnostic target: --outward checks the Windows dependency patch.
# Excluded from normal builds/tests so pristine JUCE remains supported.
if(WIN32)
    add_executable(Pro55GuiBoundaryProbe EXCLUDE_FROM_ALL GuiBoundaryProbe.cpp)
    target_compile_features(Pro55GuiBoundaryProbe PRIVATE cxx_std_20)
    target_compile_definitions(Pro55GuiBoundaryProbe PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(Pro55GuiBoundaryProbe PRIVATE juce::juce_gui_basics)
endif()
