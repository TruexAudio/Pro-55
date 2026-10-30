#pragma once

#include "dsp/Pro54Engine.h"
#include "presets/PresetLibrary.h"
#include <juce_audio_utils/juce_audio_utils.h>

class Pro55AudioProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    explicit Pro55AudioProcessor(const juce::File& bankRoot = pro55::PresetLibrary::defaultRootDirectory());
    ~Pro55AudioProcessor() override;

    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pro-55"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::Result toggleRecord(const juce::String& displayedName, bool overwriteChangedBank = false);
    bool isRecordArmed() const noexcept { return recordArmed.load(); }
    juce::String getCurrentSoundName();
    void renameCurrentSound(const juce::String&);
    unsigned getProgramRevision() const noexcept { return programRevision.load(); }
    juce::String takeStartupWarning();
    bool consumeMidiActivity() noexcept { return midiActivity.exchange(false); }

    juce::AudioProcessorValueTreeState parameters;
    pro55::PresetLibrary presets;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    pro55::Preset captureCurrentPreset();
    void selectProgram(int, bool sendDirectlyToEngine);
    void cacheProgram(int);
    void timerCallback() override;


    pro55::Pro54Engine engine;
    // Never taken by processBlock or MIDI dispatch. Protects bank copies/file work.
    juce::CriticalSection bankLock;
    int currentProgram = 0;
    int soundingProgram = 0;
    std::atomic_bool recordArmed { false };
    bool savingRecord = false;
    pro55::Preset heldPreset;
    juce::String customSoundName, startupWarning;
    bool customNameValid = false;
    std::atomic_uint programRevision { 0 };
    std::atomic_bool hostProgramChanged { false };
    static constexpr std::size_t parameterCount = 73;
    std::array<juce::RangedAudioParameter*, parameterCount> parameterPointers {};
    std::array<std::atomic<float>*, parameterCount> parameterValues {};
    std::array<uint32_t, parameterCount> endpointHandles {};
    std::array<std::array<float, parameterCount>, pro55::programCount> programValues {};
    std::array<int, 16> midiBanks {};
    std::atomic_bool midiActivity { false };
    // PitchBend and ModWheel are also driven directly by incoming MIDI pitch-wheel/CC1
    // messages inside Pro54Engine::process. Re-sending their GUI-parameter value every
    // block regardless of whether it changed would stomp a live controller's position
    // back to rest the instant the next block ran, so processBlock() only forwards these
    // two when the host/GUI value has actually moved.
    float lastSentPitchBend = 50.0f;
    float lastSentModWheel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pro55AudioProcessor)
};
