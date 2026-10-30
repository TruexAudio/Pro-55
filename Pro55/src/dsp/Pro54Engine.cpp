#include "Pro54Engine.h"
#include <utility>

namespace pro55
{
void Pro54Engine::prepare(double sampleRate, int)
{
    currentSampleRate = juce::jlimit(8000.0, 192000.0, sampleRate);
    engine.initialise(0, currentSampleRate);
}

void Pro54Engine::reset()
{
    if (currentSampleRate > 0.0)
        engine.reset();
}

void Pro54Engine::process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    jassert(currentSampleRate > 0.0);

    int cursor = 0;
    for (const auto metadata : midi)
    {
        const auto eventSample = juce::jlimit(cursor, audio.getNumSamples(), metadata.samplePosition);
        render(audio, cursor, eventSample - cursor);
        cursor = eventSample;

        const auto message = metadata.getMessage();
        if (onMidiMessage) onMidiMessage(message);
        if (message.isNoteOn())
        {
            engine.addEvent_eventIn(::Pro54::std_notes_NoteOn {
                message.getChannel() - 1,
                static_cast<float>(message.getNoteNumber()),
                message.getFloatVelocity() });
        }
        else if (message.isNoteOff())
        {
            engine.addEvent_eventIn(::Pro54::std_notes_NoteOff {
                message.getChannel() - 1,
                static_cast<float>(message.getNoteNumber()),
                message.getFloatVelocity() });
        }
        else if (message.isPitchWheel())
        {
            constexpr auto bendRangeSemitones = 48.0f;
            const auto bend = static_cast<float>(message.getPitchWheelValue() - 8192)
                            * (bendRangeSemitones / 8192.0f);
            engine.addEvent_eventIn(::Pro54::std_notes_PitchBend { message.getChannel() - 1, bend });
            // Pitch-wheel raw value is 0-16383 with 8192 = centre; the PitchBend host
            // parameter is 0-100 with 50 = centre.
            lastMidiPitchBend = static_cast<float>(message.getPitchWheelValue()) * (100.0f / 16383.0f);
        }
        else if (message.isController())
        {
            engine.addEvent_eventIn(::Pro54::std_notes_Control {
                message.getChannel() - 1,
                message.getControllerNumber(),
                static_cast<float>(message.getControllerValue()) / 127.0f });
            if (message.getControllerNumber() == 1)
                lastMidiModWheel = static_cast<float>(message.getControllerValue()) * (100.0f / 127.0f);
        }
        else if (message.getRawDataSize() <= 3)
        {
            const auto* bytes = message.getRawData();
            int packed = 0;
            for (int i = 0; i < message.getRawDataSize(); ++i)
                packed |= static_cast<int>(bytes[i]) << (16 - i * 8);

            engine.addEvent_midiIn({ packed });
        }
    }

    render(audio, cursor, audio.getNumSamples() - cursor);
}

void Pro54Engine::render(juce::AudioBuffer<float>& audio, int startSample, int numSamples)
{
    struct StereoFrame { float left, right; };
    std::array<StereoFrame, 1024> output {};
    std::array<float, 1024> input {};

    while (numSamples > 0)
    {
        const auto frames = juce::jmin(numSamples, 1024);
        engine.setInputFrames(static_cast<uint32_t>(::Pro54::EndpointHandles::externalIn),
                              input.data(), static_cast<uint32_t>(frames), 0);
        engine.advance(frames);
        engine.copyOutputFrames(static_cast<uint32_t>(::Pro54::EndpointHandles::out),
                                output.data(), static_cast<uint32_t>(frames));

        for (int frame = 0; frame < frames; ++frame)
        {
            if (audio.getNumChannels() > 0) audio.setSample(0, startSample + frame, output[frame].left);
            if (audio.getNumChannels() > 1) audio.setSample(1, startSample + frame, output[frame].right);
        }

        startSample += frames;
        numSamples -= frames;
    }
}

void Pro54Engine::setParameter(juce::StringRef id, float value)
{
    setParameter(parameterHandle(id), value);
}

uint32_t Pro54Engine::parameterHandle(juce::StringRef id)
{
    const std::string endpointName(id.text.getAddress());
    return ::Pro54::getEndpointHandleForName(endpointName);
}

void Pro54Engine::setParameter(uint32_t handle, float value)
{
    if (handle != 0)
        engine.addEvent(handle, 0, reinterpret_cast<const unsigned char*>(&value));
}

std::optional<float> Pro54Engine::takeLastPitchBendFromMidi() noexcept
{
    return std::exchange(lastMidiPitchBend, std::nullopt);
}

std::optional<float> Pro54Engine::takeLastModWheelFromMidi() noexcept
{
    return std::exchange(lastMidiModWheel, std::nullopt);
}

Pro54Engine::Diagnostics Pro54Engine::getDiagnostics() const
{
    const auto& state = engine.cmajState._state.engine;
    return { state.voices[0].gate,
             state.voices[0].oscillatorA_F,
             state.voices[0].envelopeA_Out,
             state.state.mixerOscillatorA,
             state.voices[0].oscillatorA_Phase,
             state.voices[0].filter_F_Smooth,
             state.voices[0].filter_LPF,
             state.state.volumeSmooth };
}
}
