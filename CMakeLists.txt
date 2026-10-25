cmake_minimum_required(VERSION 3.22)

# Statische MSVC-Runtime: Nutzer brauchen KEIN Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(FlipGrain VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

# ---- HIER ANPASSEN (Firmenname / Codes) -------------------------------------
juce_add_plugin(FlipGrain
    COMPANY_NAME                "YourCompany"
    COMPANY_WEBSITE             "https://example.com"
    PLUGIN_MANUFACTURER_CODE    Ycom      # 4 Zeichen, genau 1 Grossbuchstabe
    PLUGIN_CODE                 Fgrn      # 4 Zeichen, genau 1 Grossbuchstabe
    PRODUCT_NAME                "FlipGrain"
    FORMATS                     VST3
    VST3_CATEGORIES             Instrument Synth Sampler
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE)
# -----------------------------------------------------------------------------

target_sources(FlipGrain PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Analysis.cpp
    Source/Flip.cpp
    Source/ClipboardWin.cpp)

target_compile_definitions(FlipGrain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(FlipGrain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (WIN32)
    target_link_libraries(FlipGrain PRIVATE shell32 user32)
    target_compile_options(FlipGrain PRIVATE /MP)
endif()
