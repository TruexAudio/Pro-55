#include "dsp/Pro54Engine.h"
#include <cmath>
#include <iostream>
#include <memory>

int main()
{
    auto engine = std::make_unique<pro55::Pro54Engine>();
    engine->prepare(48000.0, 512);

    engine->setParameter("OscASaw", 1.0f);
    engine->setParameter("OscAFreq", 63.7795f);
    engine->setParameter("MixerOscALevel", 80.0f);
    engine->setParameter("FilterCutoff", 100.0f);
    engine->setParameter("FilterResonance", 29.1339f);
    engine->setParameter("FilterEnvAmt", 37.0079f);
    engine->setParameter("FilterKeyboardTracking", 100.0f);
    engine->setParameter("FilterAttack", 0.0f);
    engine->setParameter("FilterDecay", 68.5039f);
    engine->setParameter("FilterSustain", 100.0f);
    engine->setParameter("AmplifierAttack", 0.0f);
    engine->setParameter("AmplifierDecay", 61.4173f);
    engine->setParameter("AmplifierSustain", 100.0f);
    engine->setParameter("AmplifierRelease", 37.7953f);
    engine->setParameter("DelayON", 1.0f);
    engine->setParameter("DelayWet", 20.4724f);
    engine->setParameter("DelayFeedback", 26.7717f);
    engine->setParameter("Volume", 100.0f);
    engine->setParameter("MasterTune", 50.0f);
    engine->setParameter("PitchBend", 50.0f);
    engine->setParameter("FilterVersion", 0.0f);
    engine->setParameter("ActiveVoices", 5.0f);

    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);

    float peak = 0.0f;
    for (int block = 0; block < 8; ++block)
    {
        audio.clear();
        engine->process(audio, midi);
        midi.clear();
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            {
                const auto value = audio.getSample(channel, sample);
                if (! std::isfinite(value))
                {
                    std::cerr << "Synth produced a non-finite sample\n";
                    return 1;
                }
                peak = std::max(peak, std::abs(value));
            }
    }

    if (peak <= 1.0e-6f)
    {
        const auto diagnostics = engine->getDiagnostics();
        std::cerr << "Synth remained silent after note-on"
                  << " gate=" << diagnostics.gate
                  << " frequency=" << diagnostics.oscillatorFrequency
                  << " envelope=" << diagnostics.amplifierEnvelope
                  << " mixer=" << diagnostics.mixerLevel
                  << " phase=" << diagnostics.oscillatorPhase
                  << " filterFrequency=" << diagnostics.filterFrequency
                  << " filterOutput=" << diagnostics.filterOutput
                  << " volume=" << diagnostics.volume << '\n';
        return 1;
    }

    std::cout << "Peak: " << peak << '\n';
    return 0;
}
