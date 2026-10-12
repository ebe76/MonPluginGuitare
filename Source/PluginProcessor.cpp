#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstring>

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
{
    // Initialisation forcée à zéro de la mémoire tampon historique
    std::fill(bufferHistorique.begin(), bufferHistorique.end(), 0.0f);
}

NoteDetectorAudioProcessor::~NoteDetectorAudioProcessor() {}

void NoteDetectorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) 
{
    const juce::ScopedLock sl (lock);
    std::fill(bufferHistorique.begin(), bufferHistorique.end(), 0.0f);
}

void NoteDetectorAudioProcessor::releaseResources() {}

void NoteDetectorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // On récupère le signal mono de la guitare (canal 0)
    const float* channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();

    // Protection anti-crash si le DAW envoie un bloc plus grand que notre historique
    if (numSamples > bufferHistoriqueTaille)
        numSamples = bufferHistoriqueTaille;

    // 1. Gestion de la mémoire tampon circulaire pour les cordes graves
    int echantillonsARepasser = bufferHistoriqueTaille - numSamples;
    if (echantillonsARepasser > 0)
    {
        std::memmove (bufferHistorique.data(), bufferHistorique.data() + numSamples, echantillonsARepasser * sizeof(float));
    }
    std::memcpy (bufferHistorique.data() + echantillonsARepasser, channelData, numSamples * sizeof(float));

    // 2. Pré-Gain logiciel (Boost de sensibilité pour le sustain et les notes faibles)
    std::vector<float> amplifiedBuffer(bufferHistoriqueTaille);
    float boostFactor = 8.0f; // Multiplie le volume par 8 pour l'oreille interne du détecteur
    for (int i = 0; i < bufferHistoriqueTaille; ++i)
    {
        amplifiedBuffer[i] = bufferHistorique[i] * boostFactor;
        if (amplifiedBuffer[i] > 1.0f)  amplifiedBuffer[i] = 1.0f;
        if (amplifiedBuffer[i] < -1.0f) amplifiedBuffer[i] = -1.0f;
    }

    int bestLag = -1;
    float bestR = -1.0f;

    // 3. Calcul dynamique des limites selon la fréquence d'échantillonnage de la carte son
    int lagMinimum = static_cast<int>(sampleRate / 1500.0); // Limite haute (~1500 Hz)
    int lagMaximum = static_cast<int>(sampleRate / 50.0);   // Limite basse (~50 Hz pour inclure le Mi grave)

    // Sécurité physique pour rester dans les clous de notre historique
    if (lagMaximum > 1024) lagMaximum = 1024; 

    // 4. Algorithme d'autocorrélation adaptatif
    for (int lag = lagMinimum; lag < lagMaximum; ++lag)
    {
        float r = 0.0f;
        for (int n = 0; n < 1000; ++n) {
            r += amplifiedBuffer[n] * amplifiedBuffer[n + lag];
        }
        if (r > bestR) {
            bestR = r;
            bestLag = lag;
        }
    }

    // 5. Validation et filtrage du résultat
    const juce::ScopedLock sl (lock);
    
    // Seuil de tolérance abaissé à 0.02 pour capter les notes qui s'éteignent doucement
    if (bestLag > 0 && bestR > 0.02f) 
    {
        float freq = static_cast<float>(sampleRate) / static_cast<float>(bestLag);
        currentFrequency.store(freq);

        // Conversion mathématique de la Fréquence en Note MIDI
        int midiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(freq / 440.0f)));
        int noteIndex = midiNote % 12;

        // Dictionnaire d'accords basique selon la tonique détectée
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
