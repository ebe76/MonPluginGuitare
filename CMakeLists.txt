cmake_minimum_required(VERSION 3.15)
project(NoteDetector AUDIO_PLUGIN)

# Active le téléchargement et l'inclusion automatique de JUCE
add_subdirectory(JUCE)

juce_add_plugin(NoteDetector
    COMPANY_NAME "MonPlugin"
    PLUGIN_NAME "NoteDetector"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE MyCo
    PLUGIN_CODE Ndet
    FORMATS VST3
)

# Liste des fichiers de code source à compiler
target_sources(NoteDetector PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(NoteDetector PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(NoteDetector PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
