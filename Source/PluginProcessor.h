#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h> // NOUVEAU : Module DSP pour la FFT
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

    // Variables partagées avec le visuel
    std::atomic<float> currentFrequency { 0.0f };
    juce::String currentChordName { "Aucun accord" };
    std::vector<int> notesActivesDansLAccord; // Liste des notes détectées simultanément

private:
    std::vector<int> notesCordesAide { 40, 45, 50, 55, 59, 64 };
    juce::CriticalSection lock;

    // --- NOUVEAU : Objets FFT pour la polyphonie ---
    static const int fftOrdre = 11; // 2^11 = 2048 échantillons
    static const int fftTaille = 1 << fftOrdre;
    juce::dsp::FFT fft { fftOrdre };
    
    std::vector<float> bufferEntreeFFT { std::vector<float>(fftTaille, 0.0f) };
    std::vector<float> bufferSortieFFT { std::vector<float>(fftTaille * 2, 0.0f) };
    int indexCompteurFFT = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteDetectorAudioProcessor)
};
