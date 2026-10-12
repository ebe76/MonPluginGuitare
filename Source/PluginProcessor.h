#pragma once
#include <JuceHeader.h>
#include <vector>

class NoteDetectorAudioProcessor  : public juce::AudioProcessor
{
public:
    NoteDetectorAudioProcessor();
    ~NoteDetectorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "NoteDetector"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    void changeTuning (int tuningId);
    std::vector<int> getCurrentTuningNotes();

    // Variables partagées avec l'interface graphique (Thread-Safe)
    std::atomic<float> currentFrequency { 0.0f };
    juce::String currentChordName { "Aucun accord" };

private:
    std::vector<int> notesCordesAide { 40, 45, 50, 55, 59, 64 }; // Accordage Standard EADGBE
    juce::CriticalSection lock;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDetectorAudioProcessor)
};
