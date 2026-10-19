cmake_minimum_required(VERSION 3.22)
project(PurrOne VERSION 0.1.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Offline tools (no JUCE needed)
add_executable(purrone_render tools/render.cpp)
target_include_directories(purrone_render PRIVATE include)
add_executable(purrone_fxtest tools/fxtest.cpp)
target_include_directories(purrone_fxtest PRIVATE include)

# Plugin. Pass -DPURRONE_JUCE_DIR=/path/to/JUCE to use a local JUCE, otherwise JUCE is fetched.
option(PURRONE_BUILD_PLUGIN "Build the VST3 plugin" ON)
if(PURRONE_BUILD_PLUGIN)
  if(PURRONE_JUCE_DIR)
    add_subdirectory(${PURRONE_JUCE_DIR} juce)
  else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
  endif()

  juce_add_plugin(PurrOne
    COMPANY_NAME "Studio 87"
    PLUGIN_MANUFACTURER_CODE Spkc
    PLUGIN_CODE PurO
    FORMATS VST3 Standalone
    PRODUCT_NAME "PurrOne"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

  target_sources(PurrOne PRIVATE
    src/PluginProcessor.cpp src/PluginEditor.cpp)
  target_include_directories(PurrOne PRIVATE include src)
  target_compile_definitions(PurrOne PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
  target_link_libraries(PurrOne PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
