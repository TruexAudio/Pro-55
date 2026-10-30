#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

static void require(bool value, const char* message)
{
    if (! value) throw std::runtime_error(message);
}
static float value(Pro55AudioProcessor& p, const char* id)
{
    return p.parameters.getRawParameterValue(id)->load();
}
static void setValue(Pro55AudioProcessor& p, const char* id, float v)
{
    auto* parameter = p.parameters.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("Pro55-ProgrammerTest-" + juce::Uuid().toString());
    int code = 0;
    try
    {
        // Optional read-only compatibility check of an existing original bank.
        if (argc > 1)
        {
            auto existing = std::make_unique<Pro55AudioProcessor>(juce::File(argv[1]).getParentDirectory());
            require(existing->takeStartupWarning().isEmpty(), "Existing original bank compatibility");
            std::cout << "Existing original bank parsed: " << existing->getProgramName(0) << '\n';
        }
        auto p = std::make_unique<Pro55AudioProcessor>(root);
        require(p->getNumPrograms() == 512 && p->getCurrentProgram() == 0, "Initial programmer");
        p->setCurrentProgram(511);
        p->setCurrentProgram(512);
        require(p->getCurrentProgram() == 511, "Out-of-range program ignored");
        p->setCurrentProgram(0);
        p->renameCurrentSound("RAM rename");
        setValue(*p, "Volume", 66.0f);
        require(! root.exists(), "No files before Record commit");
        require(p->toggleRecord(p->getCurrentSoundName()).wasOk() && p->isRecordArmed(), "Record arms");
        p->setCurrentProgram(511);
        require(value(*p, "Volume") == 66.0f && p->getCurrentSoundName() == "RAM rename", "Armed selection preserves sound/name");
        p->renameCurrentSound("Captured");
        setValue(*p, "Volume", 3.0f);
        require(p->toggleRecord(p->getCurrentSoundName()).wasOk() && ! p->isRecordArmed(), "Record commits");
        require(p->presets.activeBank().presets[511].parameters.at("Volume") == 66.0f, "Snapshot frozen at arm");
        require(p->getProgramName(511) == "Captured", "Target renamed");
        auto disk = p->presets.userBankFile().loadFileAsString();
        p->setCurrentProgram(511);
        require(value(*p, "Volume") == 66.0f, "Saved program reloads");
        p->renameCurrentSound("DAW sound");
        setValue(*p, "Volume", 22.0f);
        juce::MemoryBlock state;
        p->getStateInformation(state);
        auto restored = std::make_unique<Pro55AudioProcessor>(root);
        restored->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(value(*restored, "Volume") == 22.0f && restored->getCurrentSoundName() == "DAW sound", "Project recalls actual edited sound/name");
        require(restored->getCurrentProgram() == 511 && ! restored->isRecordArmed(), "Project selection/state");
        require(restored->presets.userBankFile().loadFileAsString() == disk, "Project restore/rename never writes bank");
        restored->setCurrentProgram(511);
        require(value(*restored, "Volume") == 66.0f && restored->getCurrentSoundName() == "Captured", "Project does not replace bank");

        p->prepareToPlay(48000.0, 128);
        juce::AudioBuffer<float> audio(2, 128);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 0, 3), 30);
        midi.addEvent(juce::MidiMessage::programChange(1, 127), 40);
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 40);
        p->processBlock(audio, midi);
        require(p->getCurrentProgram() == 511 && value(*p, "Volume") == 66.0f, "CC0 plus program change without editor");
        midi.clear();
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 0, 4), 0);
        midi.addEvent(juce::MidiMessage::programChange(1, 0), 1);
        p->processBlock(audio, midi);
        require(p->getCurrentProgram() == 511, "Invalid MIDI bank must not wrap");
        require(p->toggleRecord("Held MIDI").wasOk(), "Arm before MIDI navigation");
        midi.clear();
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 0, 0), 0);
        midi.addEvent(juce::MidiMessage::programChange(1, 0), 10);
        p->processBlock(audio, midi);
        require(p->getCurrentProgram() == 0 && value(*p, "Volume") == 66.0f
            && p->getCurrentSoundName() == "Held MIDI", "Armed MIDI changes destination only");

        // A competing instance saves; cancellation/failure must leave this capture armed.
        require(restored->toggleRecord("Other instance").wasOk(), "Other instance arm");
        require(restored->toggleRecord("Other instance").wasOk(), "Other instance commit");
        require(p->toggleRecord("Held MIDI").failed() && p->isRecordArmed(), "Conflict preserves held sound");
        require(p->toggleRecord("Held MIDI", true).wasOk(), "Confirmed conflict save");
        require(p->presets.activeBank().presets[0].parameters.at("Volume") == 66.0f, "Held values survive conflict");

        // The engine invokes the programmer hook exactly between rendered spans.
        auto timing = std::make_unique<pro55::Pro54Engine>();
        timing->prepare(48000, 128);
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < 128; ++s) audio.setSample(c, s, 42.0f);
        bool sawProgramEvent = false;
        timing->onMidiMessage = [&](const juce::MidiMessage& message)
        {
            if (message.isProgramChange())
            {
                sawProgramEvent = true;
                require(audio.getSample(0, 49) != 42.0f && audio.getSample(0, 50) == 42.0f,
                        "Program callback occurs at event sample, not block start/end");
            }
        };
        midi.clear();
        midi.addEvent(juce::MidiMessage::programChange(1, 1), 50);
        timing->process(audio, midi);
        require(sawProgramEvent, "Program event hook called");

        // Old native states without a soundName still restore, with bounded indices.
        auto oldState = p->parameters.copyState();
        oldState.removeProperty("soundName", nullptr);
        oldState.setProperty("program", 9000, nullptr);
        juce::MemoryBlock oldBytes;
        juce::AudioProcessor::copyXmlToBinary(*oldState.createXml(), oldBytes);
        p->setStateInformation(oldBytes.getData(), static_cast<int>(oldBytes.getSize()));
        require(p->getCurrentProgram() == 511 && ! p->isRecordArmed(), "Old native state clamps program");

        // Exercise editor construction/destruction after the metadata workflow.
        std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
        require(editor != nullptr, "Native editor constructs");
        auto button = [&](const char* id) -> juce::ToggleButton&
        {
            auto* found = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID(id));
            require(found != nullptr, "Programmer button found");
            return *found;
        };
        button("file").onClick();
        button("program-2").onClick();
        require(pro55::programAddress(p->getCurrentProgram()) == 288, "FILE changes hundreds digit only");
        require(button("file").getToggleState() && ! button("bank").getToggleState(), "FILE latch lamp");
        button("bank").onClick();
        button("program-3").onClick();
        require(pro55::programAddress(p->getCurrentProgram()) == 238, "BANK changes tens digit only");
        require(! button("file").getToggleState() && button("bank").getToggleState(), "Exclusive BANK latch");
        button("bank").onClick();
        button("program-4").onClick();
        require(pro55::programAddress(p->getCurrentProgram()) == 234, "Unlatched buttons select units");
        auto* picker = dynamic_cast<juce::ComboBox*>(editor->findChildWithID("patch-list"));
        require(picker != nullptr && picker->getItemText(0).startsWith("111")
            && picker->getItemText(511).startsWith("888"), "Browser labels original addresses");
        auto* digits = editor->findChildWithID("program-digits");
        require(digits != nullptr, "Digit hit area found");
        const juce::Point<float> down(1.0f, 4.0f);
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent press(juce::Desktop::getInstance().getMainMouseSource(), down,
            juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, digits, digits, now, down, now, 1, false);
        const juce::MouseEvent drag(juce::Desktop::getInstance().getMainMouseSource(), {1.0f, -60.0f},
            juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, digits, digits, now, down, now, 1, true);
        digits->mouseDown(press);
        digits->mouseDrag(drag);
        require(pro55::programAddress(p->getCurrentProgram()) == 834, "Dragging hundreds clamps without changing other digits");
        auto* name = dynamic_cast<juce::TextEditor*>(editor->findChildWithID("patch-name"));
        require(name != nullptr, "Name editor found");
        require(name->keyPressed(juce::KeyPress(juce::KeyPress::F2Key)) && ! name->isReadOnly(), "F2 starts rename");
        name->setText("UI rename", false);
        name->onReturnKey();
        require(p->getCurrentSoundName() == "UI rename" && p->getProgramName(p->getCurrentProgram()) == "UI rename", "Return commits rename in RAM");
        name->keyPressed(juce::KeyPress(juce::KeyPress::F2Key));
        name->setText("Cancelled", false);
        name->onEscapeKey();
        require(p->getCurrentSoundName() == "UI rename", "Escape restores name");
        button("record").onClick();
        button("program-5").onClick();
        require(p->isRecordArmed() && p->getCurrentSoundName() == "UI rename", "UI armed navigation retains name");
        button("record").onClick();
        require(! p->isRecordArmed() && p->getProgramName(p->getCurrentProgram()) == "UI rename", "UI second Record stores capture");
        editor.reset();
        std::cout << "All processor/programmer tests passed\n";
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; code = 1; }
    if (root.getParentDirectory() == juce::File::getSpecialLocation(juce::File::tempDirectory)
        && root.getFileName().startsWith("Pro55-ProgrammerTest-")) root.deleteRecursively();
    return code;
}
