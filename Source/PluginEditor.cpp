#include "PluginProcessor.h"
#include "PluginEditor.h"

NoteDetectorAudioProcessorEditor::NoteDetectorAudioProcessorEditor (NoteDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    tuningSelector.addItem ("Standard (E A D G B E)", 1);
    tuningSelector.addItem ("Drop D (D A D G B E)",   2);
    tuningSelector.addItem ("Open D (D A D F# A D)", 3);
    tuningSelector.setSelectedId (1);
    addAndMakeVisible (tuningSelector);

    tuningSelector.onChange = [this] {
        processor.changeTuning (tuningSelector.getSelectedId());
        repaint();
    };

    setSize (800, 300);
    startTimerHz (20); // 20 rafraîchissements par seconde (1 image = 50ms)
}

NoteDetectorAudioProcessorEditor::~NoteDetectorAudioProcessorEditor() {}

void NoteDetectorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker());
    auto zoneFenetre = getLocalBounds().toFloat();
    juce::Rectangle<float> manche (10.0f, zoneFenetre.getCentreY() - 60.0f, zoneFenetre.getWidth() - 20.0f, 120.0f);
    
    g.setColour (juce::Colours::brown.darker (0.3f));
    g.fillRect (manche);

    int nbFrettes = 24;
    int nbCordes = 6;
    float espaceCorde = manche.getHeight() / (nbCordes + 1);

    // Dessin de tous les repères de touche (Inlays)
    g.setColour (juce::Colours::whitesmoke.withAlpha (0.7f));
    for (int caseRepere = 1; caseRepere <= nbFrettes; ++caseRepere)
    {
        bool aRepereSimple = (caseRepere == 3 || caseRepere == 5 || caseRepere == 7 || caseRepere == 9 || 
                              caseRepere == 15 || caseRepere == 17 || caseRepere == 19 || caseRepere == 21 || caseRepere == 23);
        bool aRepereDouble = (caseRepere == 12 || caseRepere == 24);

        if (aRepereSimple || aRepereDouble)
        {
            float posFretteActuelle = 1.0f - std::pow (1.0f / 1.059463f, caseRepere);
            float posFrettePrecedente = 1.0f - std::pow (1.0f / 1.059463f, caseRepere - 1);
            float constanteEchelle = 0.75f;
            float xCentreCase = (manche.getX() + (posFretteActuelle / constanteEchelle) * manche.getWidth() + manche.getX() + (posFrettePrecedente / constanteEchelle) * manche.getWidth()) / 2.0f;

            if (aRepereSimple) {
                g.fillEllipse (xCentreCase - 4.0f, manche.getCentreY() - 4.0f, 8.0f, 8.0f);
            } else if (aRepereDouble) {
                g.fillEllipse (xCentreCase - 4.0f, (manche.getY() + 3.0f * espaceCorde) - 4.0f, 8.0f, 8.0f);
                g.fillEllipse (xCentreCase - 4.0f, (manche.getY() + 4.0f * espaceCorde) - 4.0f, 8.0f, 8.0f);
            }
        }
    }

    // Dessin des Frettes
    g.setColour (juce::Colours::lightgrey);
    for (int frette = 0; frette <= nbFrettes; ++frette) {
        float positionRelative = 1.0f - std::pow (1.0f / 1.059463f, frette);
        float xFrette = manche.getX() + (positionRelative / 0.75f) * manche.getWidth();
        if (xFrette <= manche.getRight()) g.drawVerticalLine (static_cast<int>(xFrette), manche.getY(), manche.getBottom());
    }

    // Dessin des Cordes
    g.setColour (juce::Colours::silver);
    for (int corde = 0; corde < nbCordes; ++corde) {
        float yCorde = manche.getY() + (corde + 1) * espaceCorde;
        g.drawLine (manche.getX(), yCorde, manche.getRight(), yCorde, 0.5f + (corde * 0.4f));
    }

    // --- 4. LOGIQUE DE PERSISTANCE DE LA NOTE ---
    float freqActuelle = processor.currentFrequency.load();

    if (freqActuelle > 60.0f && freqActuelle < 2000.0f) 
    {
        // Signal valide détecté : on met à jour la mémoire et on réinitialise le compteur
        derniereFrequenceValide = freqActuelle;
        compteurImages = 0;
    }
    else
    {
        // Plus de signal : on commence à compter le temps qui passe
        compteurImages++;
        
        // Si le silence dure depuis plus de 500ms (10 images), on efface l'affichage
        if (compteurImages > limiteMaintien)
        {
            derniereFrequenceValide = 0.0f;
        }
    }

    // Dessin de la pastille rouge basée sur la note mémorisée
    if (derniereFrequenceValide > 60.0f && derniereFrequenceValide < 2000.0f) 
    {
        int midiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(derniereFrequenceValide / 440.0f)));
        std::vector<int> notesCordesAide = processor.getCurrentTuningNotes();
        
        for (int corde = 0; corde < nbCordes; ++corde) {
            int diff = midiNote - notesCordesAide[corde];
            if (diff >= 0 && diff <= nbFrettes) {
                float posFretteActuelle = 1.0f - std::pow (1.0f / 1.059463f, diff);
                float posFrettePrecedente = (diff == 0) ? 0.0f : 1.0f - std::pow (1.0f / 1.059463f, diff - 1);
                float xPastille = (diff == 0) ? manche.getX() : (manche.getX() + (posFretteActuelle / 0.75f) * manche.getWidth() + manche.getX() + (posFrettePrecedente / 0.75f) * manche.getWidth()) / 2.0f;
                float yPastille = manche.getY() + (corde + 1) * espaceCorde;

                if (xPastille <= manche.getRight()) {
                    g.setColour (juce::Colours::red);
                    g.fillEllipse (xPastille - 9.0f, yPastille - 9.0f, 18.0f, 18.0f);
                    g.setColour (juce::Colours::white);
                    g.drawEllipse (xPastille - 9.0f, yPastille - 9.0f, 18.0f, 18.0f, 1.5f);
                }
            }
        }
    }

    // Affichage du nom de l'accord détecté
    g.setFont (24.0f);
    g.setColour (juce::Colours::cyan);
    g.drawText (processor.currentChordName, 20, 15, 300, 30, juce::Justification::left);
}

void NoteDetectorAudioProcessorEditor::resized()
{
    tuningSelector.setBounds (getWidth() - 210, 15, 190, 30);
}
