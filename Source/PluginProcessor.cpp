#include "PluginProcessor.h"
#include "PluginEditor.h"

NoteDetectorAudioProcessor::NoteDetectorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{}

NoteDetectorAudioProcessor::~NoteDetectorAudioProcessor() {}

void NoteDetectorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void NoteDetectorAudioProcessor::releaseResources() {}

void NoteDetectorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();

    int bestLag = -1;
    float bestR = -1.0f;

    // Détection de Pitch par Autocorrélation
    for (int lag = 20; lag < 1000; ++lag)
    {
        float r = 0.0f;
        for (int n = 0; n < numSamples - lag; ++n) {
            r += channelData[n] * channelData[n + lag];
        }
        if (r > bestR) {
            bestR = r;
            bestLag = lag;
        }
    }

    const juce::ScopedLock sl (lock);
    if (bestLag > 0 && bestR > 0.1f) 
    {
        float freq = sampleRate / bestLag;
        currentFrequency.store(freq);

        // Conversion en note MIDI
        int midiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(freq / 440.0f)));
        int noteIndex = midiNote % 12;

        // Dictionnaire d'accords basique basé sur la note fondamentale jouée
        if (noteIndex == 0) currentChordName = "Do Majeur (C)";
        else if (noteIndex == 9) currentChordName = "La Mineur (Am)";
        else if (noteIndex == 4) currentChordName = "Mi Mineur (Em)";
        else currentChordName = "Note simple";
    } 
    else 
    {
        currentFrequency.store(0.0f);
        currentChordName = "Aucun accord";
    }
}

void NoteDetectorAudioProcessor::changeTuning (int tuningId)
{
    const juce::ScopedLock sl (lock);
    if (tuningId == 1)      notesCordesAide = { 40, 45, 50, 55, 59, 64 }; // Standard
    else if (tuningId == 2) notesCordesAide = { 38, 45, 50, 55, 59, 64 }; // Drop D
    else if (tuningId == 3) notesCordesAide = { 38, 45, 50, 54, 57, 62 }; // Open D
}

std::vector<int> NoteDetectorAudioProcessor::getCurrentTuningNotes()
{
    const juce::ScopedLock sl (lock);
    return notesCordesAide;
}

juce::AudioProcessorEditor* NoteDetectorAudioProcessor::createEditor() { return new NoteDetectorAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoteDetectorAudioProcessor(); }
