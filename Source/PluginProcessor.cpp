#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

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
    notesActivesDansLAccord.clear();
}

NoteDetectorAudioProcessor::~NoteDetectorAudioProcessor() {}
void NoteDetectorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) { notesActivesDansLAccord.clear(); }
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

    // 1. Remplissage du buffer de la FFT
    for (int i = 0; i < numSamples; ++i)
    {
        bufferEntreeFFT[indexCompteurFFT] = channelData[i];
        indexCompteurFFT++;

        // Dès qu'on a accumulé 2048 échantillons, on lance l'analyse fréquentielle
        if (indexCompteurFFT >= fftTaille)
        {
            indexCompteurFFT = 0;

            // Préparation des données complexes pour la FFT
            std::fill (bufferSortieFFT.begin(), bufferSortieFFT.end(), 0.0f);
            std::memcpy (bufferSortieFFT.data(), bufferEntreeFFT.data(), fftTaille * sizeof(float));

            // Exécution de la FFT (Transformée de Fourier Rapide)
            fft.performRealOnlyForwardTransform (bufferSortieFFT.data());

            // Tableau d'énergie pour les 12 notes de la gamme (Do, Do#, Ré...)
            std::vector<float> intensiteNotes(12, 0.0f);
            float energieTotale = 0.0f;

            // 2. Analyse des bandes de fréquences générées par la FFT
            for (int bin = 1; bin < fftTaille / 2; ++bin)
            {
                // Calcul de la magnitude (volume) de cette fréquence précise
                float reel = bufferSortieFFT[2 * bin];
                float imaginaire = bufferSortieFFT[2 * bin + 1];
                float magnitude = std::sqrt (reel * reel + imaginaire * imaginaire);

                float frequenceHz = bin * static_cast<float>(sampleRate) / static_cast<float>(fftTaille);

                // On se concentre sur la plage utile de la guitare (70Hz à 1200Hz)
                if (frequenceHz >= 70.0f && frequenceHz <= 1200.0f)
                {
                    // Formule pour trouver la note de musique correspondante
                    float noteMidiFloat = 69.0f + 12.0f * std::log2 (frequenceHz / 440.0f);
                    int noteMidi = static_cast<int>(std::round (noteMidiFloat));
                    int indexGamme = noteMidi % 12;

                    intensiteNotes[indexGamme] += magnitude;
                    energieTotale += magnitude;
                }
            }

            // 3. Identification des notes dominantes (Seuil de tolérance)
            std::vector<int> notesTrouvees;
            if (energieTotale > 2.0f) // Seuil minimum pour ignorer les bruits parasites
            {
                for (int i = 0; i < 12; ++i)
                {
                    // Si la note représente plus de 18% de l'énergie globale de l'accord, on la valide
                    if ((intensiteNotes[i] / energieTotale) > 0.18f)
                    {
                        notesTrouvees.push_back(i);
                    }
                }
            }

            // 4. Moteur de reconnaissance d'accords (Recherche de combinaisons)
            const juce::ScopedLock sl (lock);
            notesActivesDansLAccord.clear();
            currentChordName = "Note simple / Inconnu";

            // Tri pour faciliter les comparaisons
            std::sort(notesTrouvees.begin(), notesTrouvees.end());

            if (notesTrouvees.size() >= 2)
            {
                // Vérification Do Majeur (C) : Contient Do(0), Mi(4), Sol(7)
                if (std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 0) &&
                    std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 4) &&
                    std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 7))
                {
                    currentChordName = "Do Majeur (C)";
                    notesActivesDansLAccord = { 48, 52, 55 }; // Simule l'affichage des ronds sur le manche
                }
                // Vérification La Mineur (Am) : Contient La(9), Do(0), Mi(4)
                else if (std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 9) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 0) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 4))
                {
                    currentChordName = "La Mineur (Am)";
                    notesActivesDansLAccord = { 45, 48, 52 };
                }
                // Vérification Mi Mineur (Em) : Contient Mi(4), Sol(7), Si(11)
                else if (std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 4) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 7) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 11))
                {
                    currentChordName = "Mi Mineur (Em)";
                    notesActivesDansLAccord = { 40, 43, 47 };
                }
                // Vérification Ré Majeur (D) : Contient Ré(2), Fa#(6), La(9)
                else if (std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 2) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 6) &&
                         std::binary_search(notesTrouvees.begin(), notesTrouvees.end(), 9))
                {
                    currentChordName = "Ré Majeur (D)";
                    notesActivesDansLAccord = { 50, 54, 57 };
                }
            }
            else if (notesTrouvees.size() == 1)
            {
                // Si une seule note se dégage, on transmet sa fréquence brute pour le mode monophonique classique
                int noteMidiUnique = notesTrouvees[0] + 60; // Base octave 4
                currentFrequency.store (440.0f * std::pow(2.0f, (noteMidiUnique - 69.0f) / 12.0f));
                currentChordName = "Note Seule";
            }
            else
            {
                currentFrequency.store(0.0f);
                currentChordName = "Aucun accord";
            }
        }
    }
}

void NoteDetectorAudioProcessor::changeTuning (int tuningId)
{
    const juce::ScopedLock sl (lock);
    if (tuningId == 1)      notesCordesAide = { 40, 45, 50, 55, 59, 64 };
    else if (tuningId == 2) notesCordesAide = { 38, 45, 50, 55, 59, 64 };
    else if (tuningId == 3) notesCordesAide = { 38, 45, 50, 54, 57, 62 };
}

std::vector<int> NoteDetectorAudioProcessor::getCurrentTuningNotes()
{
    const juce::ScopedLock sl (lock);
    return notesCordesAide;
}

juce::AudioProcessorEditor* NoteDetectorAudioProcessor::createEditor() { return new NoteDetectorAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoteDetectorAudioProcessor(); }
