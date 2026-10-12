cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0135 NEW)

# Déclaration du projet C++
project(NoteDetector VERSION 1.0.0 LANGUAGES CXX)

# Ajout du framework JUCE
add_subdirectory(JUCE)

# Création du plugin VST3
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

# Configuration des fichiers sources (Chemins absolus sécurisés)
target_sources(NoteDetector PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.h"
)

# CORRECTION : On désactive la compatibilité VST2 pour éliminer l'erreur vstfxstore.h
target_compile_definitions(NoteDetector PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0  # Supprime la dépendance aux fichiers VST2 manquants
)

target_link_libraries(NoteDetector PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
