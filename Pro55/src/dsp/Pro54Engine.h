#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>
#include "generated/Pro54Generated.h"

namespace pro55
{
class Pro54Engine
{
public:
    struct Diagnostics
    {
        float gate = 0.0f;
        float oscillatorFrequency = 0.0f;
        float amplifierEnvelope = 0.0f;
        float mixerLevel = 0.0f;
        float oscillatorPhase = 0.0f;
        float filterFrequency = 0.0f;
        float filterOutput = 0.0f;
        float volume = 0.0f;
    };

    void prepare(double sampleRate, int maximumBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void setParameter(juce::StringRef id, float value);
    static uint32_t parameterHandle(juce::StringRef id);
    void setParameter(uint32_t handle, float value);
    // Called at the event's sample position, before dispatching that MIDI event.
    std::function<void(const juce::MidiMessage&)> onMidiMessage;
    Diagnostics getDiagnostics() const;

    // The most recent pitch-bend / mod-wheel value seen in raw MIDI during the last
    // process() call, already scaled to the PitchBend/ModWheel host parameters' 0-100
    // range, or nullopt if none arrived. Each call consumes the value, so the caller
    // can feed it straight back into the matching AudioParameterFloat to make the GUI
    // wheels track a physical MIDI controller rather than only the on-screen mouse.
    std::optional<float> takeLastPitchBendFromMidi() noexcept;
    std::optional<float> takeLastModWheelFromMidi() noexcept;

private:
    void render(juce::AudioBuffer<float>&, int startSample, int numSamples);

    ::Pro54 engine;
    double currentSampleRate = 0.0;
    std::optional<float> lastMidiPitchBend;
    std::optional<float> lastMidiModWheel;
};
}
