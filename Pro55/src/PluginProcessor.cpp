#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <utility>
#include <limits>

namespace
{
constexpr std::array parameterIds {
    "PolyModFilterEnv", "PolyModOscB", "PolyModFreqA", "PolyModPWA", "PolyModFilt",
    "OscAFreq", "OscASaw", "OscAPulse", "OscAPW", "OscASync", "MixerOscALevel",
    "MixerOscBLevel", "MixerNoiseLevel", "ExternalInputLevel", "FilterCutoff",
    "FilterResonance", "FilterEnvAmt", "FilterKeyboardTracking", "FilterAttack",
    "FilterDecay", "FilterSustain", "FilterRelease", "DelayTime", "DelaySpread",
    "DelayDepth", "DelayRate", "DelayFeedback", "DelayHiCut", "DelayLoCut", "DelayINV",
    "DelayON", "DelayWet", "DelaySync", "DelayMidi", "LfoMidiSync", "LfoFrequency",
    "LfoShapeSaw", "LfoShapeTri", "LfoShapePulse", "OscBFreq", "OscBFreqFine",
    "OscBShapeSaw", "OscBShapeTri", "OscBShapePulse", "OscBPWAmount", "OscBSubOsc",
    "OscBKKeyboardTracking", "WheelModulationLfoNoise", "WheelModulationFreqOscA",
    "WheelModulationFreqOscB", "WheelModulationPWA", "WheelModulationPWB",
    "WheelModulationFilter", "Glide", "Unison", "AmplifierAttack", "AmplifierDecay",
    "AmplifierSustain", "AmplifierRelease", "Release", "Velocity", "Repeat", "Drone",
    "FilterHPF", "FilterInvertEnv", "Analog", "MasterTune", "Volume", "ModWheel",
    "PitchBend", "FilterVersion", "ActiveVoices", "TestTone"
};

bool isSwitch(std::string_view id)
{
    static constexpr std::array switches {
        "PolyModFreqA", "PolyModPWA", "PolyModFilt", "OscASaw", "OscAPulse", "OscASync",
        "DelayINV", "DelayON", "DelaySync", "DelayMidi", "LfoMidiSync", "LfoShapeSaw",
        "LfoShapeTri", "LfoShapePulse", "OscBShapeSaw", "OscBShapeTri", "OscBShapePulse",
        "OscBSubOsc", "OscBKKeyboardTracking", "WheelModulationFreqOscA",
        "WheelModulationFreqOscB", "WheelModulationPWA", "WheelModulationPWB",
        "WheelModulationFilter", "Unison", "Release", "Velocity", "Repeat", "Drone",
        "FilterHPF", "FilterInvertEnv", "TestTone"
    };
    return std::any_of(switches.begin(), switches.end(),
                       [id](const char* entry) { return std::string_view(entry) == id; });
}

float parameterDefault(std::string_view id)
{
    if (id == "MasterTune" || id == "PitchBend") return 50.0f;
    if (id == "Volume") return 100.0f;
    return 0.0f;
}
}

Pro55AudioProcessor::Pro55AudioProcessor(const juce::File& bankRoot)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Pro54State", createParameterLayout()),
      presets(bankRoot)
{
    auto factoryJson = juce::String::fromUTF8(BinaryData::Factory_pro55bank,
                                               BinaryData::Factory_pro55bankSize);
    pro55::Bank factory;
    auto factoryStatus = pro55::Bank::fromJson(juce::JSON::parse(factoryJson), factory);
    jassert(factoryStatus.wasOk());
    if (factoryStatus.wasOk())
        presets.setFactoryBank(std::move(factory));

    const auto presetStatus = presets.load();
    if (presetStatus.failed()) startupWarning = presetStatus.getErrorMessage();
    static_assert(parameterIds.size() == parameterCount);
    for (std::size_t i = 0; i < parameterCount; ++i)
    {
        parameterPointers[i] = parameters.getParameter(parameterIds[i]);
        parameterValues[i] = parameters.getRawParameterValue(parameterIds[i]);
        endpointHandles[i] = pro55::Pro54Engine::parameterHandle(parameterIds[i]);
    }
    for (int i = 0; i < pro55::programCount; ++i) cacheProgram(i);
    setCurrentProgram(0);
    engine.onMidiMessage = [this](const juce::MidiMessage& message)
    {
        const auto channel = static_cast<std::size_t>(juce::jlimit(1, 16, message.getChannel()) - 1);
        if (message.isController() && message.getControllerNumber() == 0)
            midiBanks[channel] = message.getControllerValue();
        else if (message.isProgramChange() && midiBanks[channel] < 4)
            selectProgram(midiBanks[channel] * 128 + message.getProgramChangeNumber(), true);
    };
    startTimerHz(20);
}

Pro55AudioProcessor::~Pro55AudioProcessor()
{
    stopTimer();
}

void Pro55AudioProcessor::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    engine.prepare(sampleRate, maximumBlockSize);
    lastSentPitchBend = lastSentModWheel = std::numeric_limits<float>::quiet_NaN();
}

void Pro55AudioProcessor::releaseResources()
{
    engine.reset();
}

bool Pro55AudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Pro55AudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    const juce::ScopedLock stateGuard(getCallbackLock());
    juce::ScopedNoDenormals guard;
    keyboardState.processNextMidiBuffer(midi, 0, audio.getNumSamples(), true);
    if (! midi.isEmpty())
        midiActivity.store(true);
    for (std::size_t i = 0; i < parameterCount; ++i)
    {
        const auto id = parameterIds[i];
        const auto value = parameterValues[i]->load();
        if (std::string_view(id) == "PitchBend")
        {
            if (value == lastSentPitchBend)
                continue;
            lastSentPitchBend = value;
        }
        else if (std::string_view(id) == "ModWheel")
        {
            if (value == lastSentModWheel)
                continue;
            lastSentModWheel = value;
        }
        engine.setParameter(endpointHandles[i], value);
    }
    engine.process(audio, midi);

    // Echo a physical MIDI controller's pitch-bend/mod-wheel position back into the host
    // parameters so the on-screen wheels move to match, not just the engine's audible
    // state. setValueNotifyingHost is safe to call from the audio thread for exactly
    // this purpose; the attached GUI slider picks the change up asynchronously.
    if (const auto bend = engine.takeLastPitchBendFromMidi())
        if (auto* parameter = parameters.getParameter("PitchBend"))
            parameter->setValueNotifyingHost(*bend / 100.0f);
    if (const auto wheel = engine.takeLastModWheelFromMidi())
        if (auto* parameter = parameters.getParameter("ModWheel"))
            parameter->setValueNotifyingHost(*wheel / 100.0f);
}

juce::AudioProcessorEditor* Pro55AudioProcessor::createEditor()
{
    return new Pro55AudioProcessorEditor(*this);
}

int Pro55AudioProcessor::getNumPrograms() { return pro55::programCount; }

int Pro55AudioProcessor::getCurrentProgram()
{
    const juce::ScopedLock lock(getCallbackLock());
    return currentProgram;
}

void Pro55AudioProcessor::cacheProgram(int index)
{
    const auto& bank = presets.activeBank();
    for (std::size_t p = 0; p < parameterCount; ++p)
    {
        const auto* parameter = parameterPointers[p];
        auto value = parameter->convertFrom0to1(parameter->getDefaultValue());
        if (juce::isPositiveAndBelow(index, static_cast<int>(bank.presets.size())))
        {
            const auto& values = bank.presets[static_cast<std::size_t>(index)].parameters;
            const auto found = values.find(parameterIds[p]);
            if (found != values.end()) value = found->second;
        }
        // Validate/clamp using the parameter's own range, including stepped values.
        programValues[static_cast<std::size_t>(index)][p] =
            parameter->convertFrom0to1(juce::jlimit(0.0f, 1.0f, parameter->convertTo0to1(value)));
    }
}

void Pro55AudioProcessor::setCurrentProgram(int index)
{
    const juce::ScopedLock lock(getCallbackLock());
    selectProgram(index, false);
}

void Pro55AudioProcessor::selectProgram(int index, bool sendDirectlyToEngine)
{
    // Caller owns the callback lock. No bank parsing, map lookup, string updates or
    // file I/O here: MIDI uses the same predecoded values at the event sample.
    if (! juce::isPositiveAndBelow(index, pro55::programCount) || savingRecord) return;
    currentProgram = index;
    if (! recordArmed)
    {
        soundingProgram = index;
        customNameValid = false;
        const auto& values = programValues[static_cast<std::size_t>(index)];
        for (std::size_t i = 0; i < parameterCount; ++i)
        {
            parameterPointers[i]->setValueNotifyingHost(parameterPointers[i]->convertTo0to1(values[i]));
            if (sendDirectlyToEngine) engine.setParameter(endpointHandles[i], values[i]);
        }
        if (sendDirectlyToEngine)
        {
            // The preset's wheel values supersede earlier controller events in
            // this block; later controller events still win in normal MIDI order.
            engine.takeLastPitchBendFromMidi();
            engine.takeLastModWheelFromMidi();
            lastSentPitchBend = parameters.getRawParameterValue("PitchBend")->load();
            lastSentModWheel = parameters.getRawParameterValue("ModWheel")->load();
        }
        else
            lastSentPitchBend = lastSentModWheel = std::numeric_limits<float>::quiet_NaN();
    }
    ++programRevision;
    hostProgramChanged = true;
}

juce::String Pro55AudioProcessor::getCurrentSoundName()
{
    const juce::ScopedLock bankGuard(bankLock);
    const juce::ScopedLock lock(getCallbackLock());
    if (recordArmed) return heldPreset.name;
    return customNameValid ? customSoundName : getProgramName(soundingProgram);
}

void Pro55AudioProcessor::renameCurrentSound(const juce::String& text)
{
    const juce::ScopedLock bankGuard(bankLock);
    const auto name = text.trim().substring(0, 18);
    if (name.isEmpty()) return;
    const juce::ScopedLock lock(getCallbackLock());
    if (savingRecord) return;
    if (recordArmed) heldPreset.name = name;
    else
    {
        presets.renamePreset(static_cast<std::size_t>(currentProgram), name);
        customSoundName = name;
        customNameValid = true;
    }
    ++programRevision;
    hostProgramChanged = true;
}

juce::Result Pro55AudioProcessor::toggleRecord(const juce::String& displayedName, bool overwriteChangedBank)
{
    const juce::ScopedLock bankGuard(bankLock);
    auto name = displayedName.trim();
    int destination = 0;
    pro55::Preset captured;
    std::optional<pro55::PresetLibrary> candidate;
    {
        const juce::ScopedLock lock(getCallbackLock());
        if (savingRecord) return juce::Result::fail("A bank save is already in progress");
        if (! recordArmed)
        {
            heldPreset = captureCurrentPreset();
            if (name.isNotEmpty()) heldPreset.name = name;
            recordArmed = true;
            ++programRevision;
            return juce::Result::ok();
        }
        if (name.isEmpty()) return juce::Result::fail("Enter a name for the preset");
        heldPreset.name = name;
        destination = currentProgram;
        captured = heldPreset;
        savingRecord = true;
    }
    candidate = presets;
    // Disk access is deliberately outside the audio callback lock. The audio
    // thread uses the unchanged cache while the candidate bank is being saved.
    const auto status = candidate->savePreset(static_cast<std::size_t>(destination), captured, overwriteChangedBank);
    {
        const juce::ScopedLock lock(getCallbackLock());
        savingRecord = false;
        // On failure only conflict metadata changes; the working bank stays intact.
        std::swap(presets, *candidate);
        if (status.failed()) return status;
        cacheProgram(destination);
        currentProgram = soundingProgram = destination;
        customSoundName = captured.name;
        customNameValid = true;
        recordArmed = false;
        ++programRevision;
        hostProgramChanged = true;
    }
    return juce::Result::ok();
}

const juce::String Pro55AudioProcessor::getProgramName(int index)
{
    const juce::ScopedLock bankGuard(bankLock);
    const auto& bank = presets.activeBank();
    return juce::isPositiveAndBelow(index, static_cast<int>(bank.presets.size()))
        ? bank.presets[static_cast<std::size_t>(index)].name : juce::String();
}

void Pro55AudioProcessor::changeProgramName(int index, const juce::String& name)
{
    const juce::ScopedLock bankGuard(bankLock);
    const juce::ScopedLock lock(getCallbackLock());
    if (savingRecord || name.trim().isEmpty()) return;
    if (index == currentProgram) renameCurrentSound(name);
    else
    {
        presets.renamePreset(static_cast<std::size_t>(index), name.trim().substring(0, 18));
        ++programRevision;
        hostProgramChanged = true;
    }
}

juce::String Pro55AudioProcessor::takeStartupWarning()
{
    return std::exchange(startupWarning, {});
}

void Pro55AudioProcessor::timerCallback()
{
    if (hostProgramChanged.exchange(false))
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
}

void Pro55AudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const juce::ScopedLock bankGuard(bankLock);
    const juce::ScopedLock lock(getCallbackLock());
    auto state = parameters.copyState();
    state.setProperty("program", currentProgram, nullptr);
    state.setProperty("soundName", getCurrentSoundName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}

void Pro55AudioProcessor::setStateInformation(const void* data, int size)
{
    auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || ! xml->hasTagName(parameters.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xml);
    const juce::ScopedLock bankGuard(bankLock);
    const juce::ScopedLock lock(getCallbackLock());
    if (savingRecord) return;
    parameters.replaceState(state);
    lastSentPitchBend = lastSentModWheel = std::numeric_limits<float>::quiet_NaN();
    recordArmed = false;
    currentProgram = soundingProgram = juce::jlimit(0, pro55::programCount - 1,
        static_cast<int>(state.getProperty("program", 0)));
    customSoundName = state.getProperty("soundName", getProgramName(currentProgram)).toString();
    customNameValid = true;
    ++programRevision;
    hostProgramChanged = true;
}

juce::AudioProcessorValueTreeState::ParameterLayout Pro55AudioProcessor::createParameterLayout()
{
    pro55::ParameterValues factoryDefaults;
    const auto factoryJson = juce::String::fromUTF8(BinaryData::Factory_pro55bank,
                                                     BinaryData::Factory_pro55bankSize);
    pro55::Bank factory;
    if (pro55::Bank::fromJson(juce::JSON::parse(factoryJson), factory).wasOk()
        && ! factory.presets.empty())
        factoryDefaults = factory.presets.front().parameters;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto id : parameterIds)
    {
        const auto found = factoryDefaults.find(id);
        const auto defaultValue = found != factoryDefaults.end() ? found->second : parameterDefault(id);
        if (isSwitch(id))
        {
            layout.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID { id, 1 }, id, defaultValue >= 0.5f));
        }
        else if (juce::String(id) == "ActiveVoices")
        {
            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID { id, 1 }, id, 1, 32,
                juce::jlimit(1, 32, juce::roundToInt(defaultValue))));
        }
        else
        {
            const auto range = juce::String(id) == "FilterVersion"
                ? juce::NormalisableRange<float>(0.0f, 1.0f, 0.5f)
                : juce::NormalisableRange<float>(0.0f, 100.0f);
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { id, 1 }, id, range, defaultValue));
        }
    }
    return layout;
}

pro55::Preset Pro55AudioProcessor::captureCurrentPreset()
{
    pro55::Preset preset;
    preset.id = juce::Uuid().toString();
    preset.name = getCurrentSoundName();
    for (auto id : parameterIds)
        preset.parameters[id] = parameters.getRawParameterValue(id)->load();
    return preset;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Pro55AudioProcessor();
}
