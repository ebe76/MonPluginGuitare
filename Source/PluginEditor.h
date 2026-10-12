#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NoteDetectorAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    NoteDetectorAudioProcessorEditor (NoteDetectorAudioProcessor&);
    ~NoteDetectorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); } // Rafraîchit l'affichage en temps réel
    juce::ComboBox tuningSelector;
    NoteDetectorAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDetectorAudioProcessorEditor)
};
