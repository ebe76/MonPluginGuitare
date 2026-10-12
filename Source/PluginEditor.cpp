#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NoteDetectorAudioProcessorEditor::NoteDetectorAudioProcessorEditor (NoteDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Configuration et remplissage du menu déroulant d'accordage
    tuningSelector.addItem ("Standard (E A D G B E)", 1);
    tuningSelector.addItem ("Drop D (D A D G B E)",   2);
    tuningSelector.setSelectedId (1);
    addAndMakeVisible (tuningSelector);

    // Action déclenchée lorsque l'utilisateur change d'accordage
    tuningSelector.onChange = [this] {
        processor.changeTuning (tuningSelector.getSelectedId());
        repaint(); // Force le rafraîchissement immédiat du dessin
    };

    setSize (800, 300); // Taille de la fenêtre du plugin VST3
    startTimerHz (20);  // 20 rafraîchissements par seconde (1 image toutes les 50ms)
}

NoteDetectorAudioProcessorEditor::~NoteDetectorAudioProcessorEditor() {}

void NoteDetectorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Dessin de l'arrière-plan et de la touche du manche
    g.fillAll (juce::Colours::darkgrey.darker());
    auto zoneFenetre = getLocalBounds().toFloat();
    juce::Rectangle<float> manche (10.0f, zoneFenetre.getCentreY() - 60.0f, zoneFenetre.getWidth() - 20.0f, 120.0f);
    
    g.setColour (juce::Colours::brown.darker (0.3f));
    g.fillRect (manche); // Couleur bois du manche

    int nbFrettes = 24;
    int nbCordes = 6;
    float espaceCorde = manche.getHeight() / (nbCordes + 1);

    // 2. Dessin de TOUS les repères de touche traditionnels (Inlays)
    g.setColour (juce::Colours::whitesmoke.withAlpha (0.7f)); // Blanc vintage légèrement transparent
    for (int caseRepere = 1; caseRepere <= nbFrettes; ++caseRepere)
    {
        // Définition des cases à repère unique et double
        bool aRepereSimple = (caseRepere == 3 || caseRepere == 5 || caseRepere == 7 || caseRepere == 9 || 
                              caseRepere == 15 || caseRepere == 17 || caseRepere == 19 || caseRepere == 21 || caseRepere == 23);
        bool aRepereDouble = (caseRepere == 12 || caseRepere == 24);

        if (aRepereSimple || aRepereDouble)
        {
            // Calcul logarithmique pour centrer les repères au milieu de la case
            float posFretteActuelle = 1.0f - std::pow (1.0f / 1.059463f, caseRepere);
            float posFrettePrecedente = 1.0f - std::pow (1.0f / 1.059463f, caseRepere - 1);
            float constanteEchelle = 0.75f;
            float xCentreCase = (manche.getX() + (posFretteActuelle / constanteEchelle) * manche.getWidth() + manche.getX() + (posFrettePrecedente / constanteEchelle) * manche.getWidth()) / 2.0f;

            if (aRepereSimple) 
            {
                // Un seul point centré sur la touche
                g.fillEllipse (xCentreCase - 4.0f, manche.getCentreY() - 4.0f, 8.0f, 8.0f);
            } 
            else if (aRepereDouble) 
            {
                // Deux points alignés verticalement (entre cordes 2/3 et 4/5)
                g.fillEllipse (xCentreCase - 4.0f, (manche.getY() + 3.0f * espaceCorde) - 4.0f, 8.0f, 8.0f);
                g.fillEllipse (xCentreCase - 4.0f, (manche.getY() + 4.0f * espaceCorde) - 4.0f, 8.0f, 8.0f);
            }
        }
    }

    // 3. Dessin des Frettes (Lignes verticales logarithmiques)
    g.setColour (juce::Colours::lightgrey);
    for (int frette = 0; frette <= nbFrettes; ++frette) 
    {
        float positionRelative = 1.0f - std::pow (1.0f / 1.059463f, frette);
        float xFrette = manche.getX() + (positionRelative / 0.75f) * manche.getWidth();
        if (xFrette <= manche.getRight()) 
            g.drawVerticalLine (static_cast<int>(xFrette), manche.getY(), manche.getBottom());
    }

    // 4. Dessin des 6 Cordes (Lignes horizontales avec épaisseurs progressives)
    g.setColour (juce::Colours::silver);
    for (int corde = 0; corde < nbCordes; ++corde) 
    {
        float yCorde = manche.getY() + (corde + 1) * espaceCorde;
        g.drawLine (manche.getX(), yCorde, manche.getRight(), yCorde, 0.5f + (corde * 0.4f));
    }

    // --- 5. LOGIQUE D'AFFICHAGE MULTI-NOTES (ACCORDS OU NOTES SEULES) ---
    std::vector<int> notesADessiner;
    
    // Si un accord est identifié dans le moteur FFT, on récupère sa liste de notes multiples
    if (processor.currentChordName != "Aucun accord" && processor.currentChordName != "Note Seule" && processor.currentChordName != "Note simple / Inconnu")
    {
        notesADessiner = processor.notesActivesDansLAccord;
        compteurImages = 0; // Réinitialise la mémoire tampon visuelle
    }
    // Sinon, on applique le lissage temporel (Hold-Time) pour les notes seules
    else
    {
        float freqActuelle = processor.currentFrequency.load();
        if (freqActuelle > 60.0f && freqActuelle < 2000.0f) 
        {
            derniereFrequenceValide = freqActuelle;
            compteurImages = 0; // Réinitialise le compteur
        }
        else
        {
            compteurImages++;
            // Maintien visuel pendant 500ms (10 images x 50ms) avant disparition
            if (compteurImages > limiteMaintien) 
                derniereFrequenceValide = 0.0f;
        }

        if (derniereFrequenceValide > 60.0f)
        {
            int midiNoteUnique = static_cast<int>(std::round(69.0f + 12.0f * std::log2(derniereFrequenceValide / 440.0f)));
            notesADessiner.push_back(midiNoteUnique);
        }
    }

    // 6. Placement et dessin des pastilles rouges actives de l'accord sur le manche
    std::vector<int> notesCordesAide = processor.getCurrentTuningNotes();
    
    for (int midiNote : notesADessiner)
    {
        for (int corde = 0; corde < nbCordes; ++corde) 
        {
            int diff = midiNote - notesCordesAide[corde];
            if (diff >= 0 && diff <= nbFrettes) 
            {
                float posFretteActuelle = 1.0f - std::pow (1.0f / 1.059463f, diff);
                float posFrettePrecedente = (diff == 0) ? 0.0f : 1.0f - std::pow (1.0f / 1.059463f, diff - 1);
                
                float xPastille = (diff == 0) ? manche.getX() : (manche.getX() + (posFretteActuelle / 0.75f) * manche.getWidth() + manche.getX() + (posFrettePrecedente / 0.75f) * manche.getWidth()) / 2.0f;
                float yPastille = manche.getY() + (corde + 1) * espaceCorde;

                // On dessine la pastille si elle ne déborde pas à droite du manche graphique
                if (xPastille <= manche.getRight()) 
                {
                    // Rond rouge
                    g.setColour (juce::Colours::red);
                    g.fillEllipse (xPastille - 9.0f, yPastille - 9.0f, 18.0f, 18.0f);
                    
                    // Contour blanc pour la lisibilité
                    g.setColour (juce::Colours::white);
                    g.drawEllipse (xPastille - 9.0f, yPastille - 9.0f, 18.0f, 18.0f, 1.5f);
                }
            }
        }
    }

    // 7. Affichage textuel du nom de l'accord détecté (en haut à gauche)
    g.setFont (24.0f);
    g.setColour (juce::Colours::cyan);
    g.drawText (processor.currentChordName, 20, 15, 400, 30, juce::Justification::left);
}

void NoteDetectorAudioProcessorEditor::resized()
{
    // Positionnement du sélecteur d'accordage en haut à droite de l'écran
    tuningSelector.setBounds (getWidth() - 210, 15, 190, 30);
}
