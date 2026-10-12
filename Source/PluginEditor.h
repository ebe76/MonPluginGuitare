#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class NoteDetectorAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    NoteDetectorAudioProcessorEditor (NoteDetectorAudioProcessor&);
    ~NoteDetectorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    juce::ComboBox tuningSelector;
    NoteDetectorAudioProcessor& processor;

    // --- VARIABLES DE PERSISTANCE VISUELLE ---
    float derniereFrequenceValide { 0.0f };
    int compteurImages { 0 };       // Compte le nombre d'images sans signal
    int limiteMaintien { 10 };      // 10 images à 20Hz = 500ms de maintien de la note

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDetectorAudioProcessorEditor)
};
