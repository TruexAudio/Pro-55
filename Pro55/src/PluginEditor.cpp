#include "PluginEditor.h"
#include "BinaryData.h"
#include <limits>

namespace
{
constexpr float sourceScale = 1.5f;

// Every control position below is authored in the original 762x358 CSS design space. Project it
// directly into the current editor bounds and round only the final rectangle edges. Snapping to
// the intermediate 1143x537 default-size grid and then scaling that snapped rectangle again makes
// half-pixel decisions at the default size grow into 1-3 physical pixels in a large/DPI-scaled
// window. That is visible for buttons because the unlit body is baked into the continuously
// stretched background while the lit state draws a separate sprite over it.
juce::Rectangle<int> layoutRect(float x, float y, float width, float height, float sx, float sy)
{
    return juce::Rectangle<float>(x * sourceScale * sx,
                                  y * sourceScale * sy,
                                  width * sourceScale * sx,
                                  height * sourceScale * sy)
        .toNearestIntEdges();
}

constexpr Pro55AudioProcessorEditor::ControlSpec controlSpecs[] {
    { "PolyModFilterEnv", 23, 23, false }, { "PolyModOscB", 61, 23, false },
    { "PolyModFreqA", 95, 26, true }, { "PolyModPWA", 115, 26, true },
    { "PolyModFilt", 135, 26, true }, { "OscAFreq", 168, 23, false },
    { "OscASaw", 202, 26, true }, { "OscAPulse", 222, 26, true },
    { "OscAPW", 244, 23, false }, { "OscASync", 281, 26, true },
    { "MixerOscALevel", 313, 23, false }, { "MixerOscBLevel", 349, 23, false },
    { "MixerNoiseLevel", 385, 23, false }, { "ExternalInputLevel", 385, 69, false },
    { "FilterCutoff", 432, 23, false }, { "FilterResonance", 468, 23, false },
    { "FilterEnvAmt", 504, 23, false }, { "FilterKeyboardTracking", 540, 23, false },
    { "FilterAttack", 432, 69, false }, { "FilterDecay", 468, 69, false },
    { "FilterSustain", 504, 69, false }, { "FilterRelease", 540, 69, false },
    { "DelayTime", 607, 23, false }, { "DelaySpread", 643, 23, false },
    { "DelayDepth", 679, 23, false }, { "DelayRate", 715, 23, false },
    { "DelayFeedback", 607, 69, false }, { "DelayHiCut", 643, 69, false },
    { "DelayLoCut", 679, 69, false }, { "DelayINV", 721, 72, true },
    { "DelayON", 613, 118, true }, { "DelayWet", 643, 115, false },
    { "DelaySync", 685, 118, true }, { "DelayMidi", 721, 118, true },
    { "LfoMidiSync", 41, 72, true }, { "LfoFrequency", 61, 69, false },
    { "LfoShapeSaw", 95, 72, true }, { "LfoShapeTri", 115, 72, true },
    { "LfoShapePulse", 135, 72, true }, { "OscBFreq", 168, 69, false },
    { "OscBFreqFine", 206, 69, false }, { "OscBShapeSaw", 240, 72, true },
    { "OscBShapeTri", 260, 72, true }, { "OscBShapePulse", 280, 72, true },
    { "OscBPWAmount", 302, 69, false }, { "OscBSubOsc", 336, 72, true },
    { "OscBKKeyboardTracking", 356, 72, true }, { "WheelModulationLfoNoise", 23, 115, false },
    { "WheelModulationFreqOscA", 55, 118, true }, { "WheelModulationFreqOscB", 75, 118, true },
    { "WheelModulationPWA", 95, 118, true }, { "WheelModulationPWB", 115, 118, true },
    { "WheelModulationFilter", 135, 118, true }, { "Glide", 168, 115, false },
    { "Unison", 212, 118, true }, { "AmplifierAttack", 432, 115, false },
    { "AmplifierDecay", 468, 115, false }, { "AmplifierSustain", 504, 115, false },
    { "AmplifierRelease", 540, 115, false }, { "Release", 386, 118, true },
    { "Velocity", 346, 118, true }, { "Repeat", 21, 72, true },
    { "Drone", 574, 118, true }, { "FilterHPF", 574, 26, true },
    { "FilterInvertEnv", 574, 72, true }, { "Analog", 302, 115, false },
    { "MasterTune", 607, 161, false }, { "Volume", 679, 161, false },
    { "TestTone", 649, 164, true }
};
}

Pro55AudioProcessorEditor::PanelKeyboard::PanelKeyboard(juce::MidiKeyboardState& state)
    : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard)
#if PRO55_ALT_GUI
    , keyboardState(state)
#endif
{
}

void Pro55AudioProcessorEditor::PanelKeyboard::setKeyMap(juce::Image image)
{
#if PRO55_ALT_GUI
    keyMap = std::move(image);
    noteIndices.clear();
    for (auto& mask : keyMasks)
        mask = {};

    if (! keyMap.isValid())
        return;

    const auto width = keyMap.getWidth();
    const auto height = keyMap.getHeight();
    noteIndices.assign(static_cast<std::size_t>(width * height), static_cast<signed char>(-1));

    std::array<int, mappedNoteCount> minX, minY, maxX, maxY;
    minX.fill(width);
    minY.fill(height);
    maxX.fill(-1);
    maxY.fill(-1);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto index = keyIndexForColour(keyMap.getPixelAt(x, y));
            if (index < 0)
                continue;

            noteIndices[static_cast<std::size_t>(y * width + x)] = static_cast<signed char>(index);
            minX[static_cast<std::size_t>(index)] = juce::jmin(minX[static_cast<std::size_t>(index)], x);
            minY[static_cast<std::size_t>(index)] = juce::jmin(minY[static_cast<std::size_t>(index)], y);
            maxX[static_cast<std::size_t>(index)] = juce::jmax(maxX[static_cast<std::size_t>(index)], x);
            maxY[static_cast<std::size_t>(index)] = juce::jmax(maxY[static_cast<std::size_t>(index)], y);
        }
    }

    for (int index = 0; index < mappedNoteCount; ++index)
    {
        if (maxX[static_cast<std::size_t>(index)] < minX[static_cast<std::size_t>(index)])
            continue;

        auto& mask = keyMasks[static_cast<std::size_t>(index)];
        mask.panelBounds = juce::Rectangle<int>::leftTopRightBottom(
            minX[static_cast<std::size_t>(index)], minY[static_cast<std::size_t>(index)],
            maxX[static_cast<std::size_t>(index)] + 1, maxY[static_cast<std::size_t>(index)] + 1);
        mask.image = juce::Image(juce::Image::ARGB, mask.panelBounds.getWidth(),
                                 mask.panelBounds.getHeight(), true);
    }

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto index = static_cast<int>(noteIndices[static_cast<std::size_t>(y * width + x)]);
            if (index < 0)
                continue;

            auto& mask = keyMasks[static_cast<std::size_t>(index)];
            mask.image.setPixelAt(x - mask.panelBounds.getX(), y - mask.panelBounds.getY(),
                                  juce::Colours::white.withAlpha(keyMap.getPixelAt(x, y).getFloatAlpha()));
        }
    }
#else
    juce::ignoreUnused(image);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::paint(juce::Graphics& graphics)
{
#if PRO55_ALT_GUI
    if (! keyMap.isValid())
        return;

    const auto scaleX = static_cast<float>(getWidth()) / static_cast<float>(keyMap.getWidth());
    const auto scaleY = static_cast<float>(getHeight()) / static_cast<float>(keyMap.getHeight());
    graphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    for (int index = 0; index < mappedNoteCount; ++index)
    {
        const auto note = firstMappedNote + index;
        auto overlay = juce::Colours::transparentBlack;
        if (keyboardState.isNoteOnForChannels(getMidiChannelsToDisplay(), note))
            overlay = findColour(keyDownOverlayColourId);
        if (hoveredNote == note)
            overlay = overlay.overlaidWith(findColour(mouseOverKeyOverlayColourId));
        if (overlay.isTransparent())
            continue;

        const auto& mask = keyMasks[static_cast<std::size_t>(index)];
        if (! mask.image.isValid())
            continue;

        graphics.setColour(overlay);
        graphics.drawImage(mask.image,
                           mask.panelBounds.toFloat().transformedBy(
                               juce::AffineTransform::scale(scaleX, scaleY)),
                           juce::RectanglePlacement::stretchToFit, true);
    }
#else
    juce::MidiKeyboardComponent::paint(graphics);
#endif
}

bool Pro55AudioProcessorEditor::PanelKeyboard::hitTest(int x, int y)
{
#if PRO55_ALT_GUI
    return noteAt({ x, y }) >= 0;
#else
    return juce::Component::hitTest(x, y);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseMove(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    const auto note = noteAt(event.getPosition());
    if (hoveredNote != note)
    {
        hoveredNote = note;
        repaint();
    }
#else
    juce::MidiKeyboardComponent::mouseMove(event);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseEnter(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    mouseMove(event);
#else
    juce::MidiKeyboardComponent::mouseEnter(event);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseExit(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    juce::ignoreUnused(event);
    hoveredNote = -1;
    repaint();
#else
    juce::MidiKeyboardComponent::mouseExit(event);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseDown(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    grabKeyboardFocus();
    updateMouseNote(event, true);
#else
    juce::MidiKeyboardComponent::mouseDown(event);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseDrag(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    updateMouseNote(event, true);
#else
    juce::MidiKeyboardComponent::mouseDrag(event);
#endif
}

void Pro55AudioProcessorEditor::PanelKeyboard::mouseUp(const juce::MouseEvent& event)
{
#if PRO55_ALT_GUI
    updateMouseNote(event, false);
#else
    juce::MidiKeyboardComponent::mouseUp(event);
#endif
}

#if PRO55_ALT_GUI
int Pro55AudioProcessorEditor::PanelKeyboard::keyIndexForColour(juce::Colour colour) const
{
    if (colour.getAlpha() == 0)
        return -1;

    constexpr int levels[] { 32, 96, 160, 224 };
    int bestIndex = -1;
    int bestDistance = std::numeric_limits<int>::max();
    for (int index = 0; index < mappedNoteCount; ++index)
    {
        const auto permuted = (index * 37) % 64;
        const auto red = levels[permuted % 4];
        const auto green = levels[(permuted / 4) % 4];
        const auto blue = levels[(permuted / 16) % 4];
        const auto dr = static_cast<int>(colour.getRed()) - red;
        const auto dg = static_cast<int>(colour.getGreen()) - green;
        const auto db = static_cast<int>(colour.getBlue()) - blue;
        const auto distance = dr * dr + dg * dg + db * db;
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = index;
        }
    }
    return bestIndex;
}

int Pro55AudioProcessorEditor::PanelKeyboard::noteAt(juce::Point<int> point) const
{
    if (! keyMap.isValid() || getWidth() <= 0 || getHeight() <= 0
        || ! getLocalBounds().contains(point))
        return -1;

    const auto mapX = juce::jlimit(0, keyMap.getWidth() - 1,
                                  point.x * keyMap.getWidth() / getWidth());
    const auto mapY = juce::jlimit(0, keyMap.getHeight() - 1,
                                  point.y * keyMap.getHeight() / getHeight());
    const auto index = static_cast<int>(noteIndices[static_cast<std::size_t>(
        mapY * keyMap.getWidth() + mapX)]);
    return index >= 0 ? firstMappedNote + index : -1;
}

float Pro55AudioProcessorEditor::PanelKeyboard::velocityAt(int note, juce::Point<int> point) const
{
    const auto index = note - firstMappedNote;
    if (index < 0 || index >= mappedNoteCount || getHeight() <= 0)
        return 1.0f;

    const auto mapY = point.y * keyMap.getHeight() / getHeight();
    const auto bounds = keyMasks[static_cast<std::size_t>(index)].panelBounds;
    return juce::jlimit(0.05f, 1.0f,
                        static_cast<float>(mapY - bounds.getY())
                            / static_cast<float>(juce::jmax(1, bounds.getHeight())));
}

void Pro55AudioProcessorEditor::PanelKeyboard::updateMouseNote(const juce::MouseEvent& event,
                                                                bool isDown)
{
    const auto nextNote = isDown ? noteAt(event.getPosition()) : -1;
    hoveredNote = noteAt(event.getPosition());
    if (nextNote == mouseNote)
        return;

    if (mouseNote >= 0)
        keyboardState.noteOff(getMidiChannel(), mouseNote, 0.0f);
    mouseNote = nextNote;
    if (mouseNote >= 0)
        keyboardState.noteOn(getMidiChannel(), mouseNote,
                             velocityAt(mouseNote, event.getPosition()));
    repaint();
}
#endif

Pro55AudioProcessorEditor::Pro55AudioProcessorEditor(Pro55AudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner),
      keyboard(owner.keyboardState)
{
    // paint() covers the complete editor with an opaque background. Advertising
    // this lets JUCE skip painting wrapper backing hidden by the editor.
    setOpaque(true);
#if PRO55_ALT_GUI
    background = juce::ImageFileFormat::loadFrom(BinaryData::panel_png,
                                                   BinaryData::panel_pngSize);
#else
    background = juce::ImageFileFormat::loadFrom(BinaryData::panel_legacy_png,
                                                   BinaryData::panel_legacy_pngSize);
#endif
    programDigits = juce::ImageFileFormat::loadFrom(BinaryData::program_digits_png,
                                                     BinaryData::program_digits_pngSize);
    voiceDigits = juce::ImageFileFormat::loadFrom(BinaryData::voice_digits_png, BinaryData::voice_digits_pngSize);
    midiLight = juce::ImageFileFormat::loadFrom(BinaryData::midi_indicator_png, BinaryData::midi_indicator_pngSize);
    bpfIndicator = juce::ImageFileFormat::loadFrom(BinaryData::bpf_indicator_png, BinaryData::bpf_indicator_pngSize);
    setResizable(true, true);
    setResizeLimits(857, 403, 2858, 1343);
    getConstrainer()->setFixedAspectRatio(1143.0 / 537.0);

    for (const auto& spec : controlSpecs)
        addControl(spec);

    for (auto* slider : std::array<juce::Slider*, 4> {
             &pitchWheel, &modWheel, &activeVoices, &filterVersion })
    {
        slider->setLookAndFeel(&panelLookAndFeel);
        addAndMakeVisible(*slider);
    }
    activeVoices.getProperties().set(juce::Identifier("voices"), true);
    filterVersion.getProperties().set(juce::Identifier("filterSelector"), true);
    activeVoices.setAlpha(0.01f);
    pitchWheel.setDoubleClickReturnValue(true, 50.0);
    specialAttachments[0] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "PitchBend", pitchWheel);
    specialAttachments[1] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "ModWheel", modWheel);
    specialAttachments[2] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "ActiveVoices", activeVoices);
    specialAttachments[3] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "FilterVersion", filterVersion);
    if (auto* filterParameter = processor.parameters.getParameter("FilterVersion"))
        filterVersion.setParameter(*filterParameter);

    keyboard.setAvailableRange(36, 96);
    keyboard.setScrollButtonsVisible(false);
#if PRO55_ALT_GUI
    keyboard.setOpaque(false);
    keyboard.setKeyMap(juce::ImageFileFormat::loadFrom(BinaryData::keyboard_keymap_png,
                                                        BinaryData::keyboard_keymap_pngSize));
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       juce::Colour(0x28383838));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       juce::Colour(0x88505050));
#else
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe6dfcf));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff181716));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff4b4035));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       juce::Colours::orange.withAlpha(0.20f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       juce::Colours::orange.withAlpha(0.45f));
#endif
    addAndMakeVisible(keyboard);

    presetName.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff330000));
    presetName.setColour(juce::TextEditor::textColourId, juce::Colours::red);
    presetName.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    presetName.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::red);
    presetName.setFont(juce::FontOptions(11.0f).withName(juce::Font::getDefaultMonospacedFontName()));
    presetName.setSelectAllWhenFocused(false);
    presetName.setReadOnly(true);
    presetName.onBrowseRequested = [this] { presetSelector.showPopup(); };
    presetName.setInputRestrictions(18);
    presetName.setTooltip("Click to browse; F2 or right-click to rename. While Record is armed, click to rename.");
    presetName.onTextChange = [this]
    {
        if (! presetName.isReadOnly()) processor.renameCurrentSound(presetName.getText());
    };
    presetName.onRenameCommitted = [this] { processor.renameCurrentSound(presetName.getText()); };
    presetName.onReturnKey = [this] { presetName.finishRename(); refreshPresetControls(); };
    presetName.onEscapeKey = [this]
    {
        presetName.finishRename(true);
        processor.renameCurrentSound(presetName.getText());
        refreshPresetControls();
    };
    addAndMakeVisible(presetName);
    presetName.setComponentID("patch-name");

    presetSelector.setAlpha(0.01f);
    // The picker only ever hosts the popup list; the name editor beneath it owns the
    // mouse. Without this it covers the name field and swallows the rename gesture.
    presetSelector.setInterceptsMouseClicks(false, false);
    presetSelector.onChange = [this]
    {
        const auto index = presetSelector.getSelectedItemIndex();
        if (index >= 0)
            processor.setCurrentProgram(index);
        refreshPresetControls();
    };
    addAndMakeVisible(presetSelector);
    presetSelector.setComponentID("patch-list");

    for (auto* button : { &saveButton, &saveAsButton, &bankButton })
    {
        button->setLookAndFeel(&panelLookAndFeel);
        // Lamps reflect Record state or the mutually exclusive FILE/BANK modes.
        button->setClickingTogglesState(false);
        addAndMakeVisible(*button);
    }
    // RECORD lights orange, FILE and BANK light grey, matching the panel artwork.
    saveButton.getProperties().set(juce::Identifier("orange"), true);
    saveAsButton.getProperties().set(juce::Identifier("grey"), true);
    bankButton.getProperties().set(juce::Identifier("grey"), true);
    saveButton.setTooltip("Record: capture sound, choose destination, press again to save");
    saveButton.setComponentID("record");
    saveAsButton.setComponentID("file");
    bankButton.setComponentID("bank");
    saveAsButton.setTooltip("FILE: select the first program digit using buttons 1-8");
    bankButton.setTooltip("BANK: select the second program digit using buttons 1-8");
    bankButton.onClick = [this]
    {
        selectedDigit = selectedDigit == 1 ? 2 : 1;
        refreshPresetControls();
    };
    saveAsButton.onClick = [this]
    {
        selectedDigit = selectedDigit == 0 ? 2 : 0;
        refreshPresetControls();
    };
    saveButton.onClick = [this] { recordPressed(); };

    for (int index = 0; index < static_cast<int>(programButtons.size()); ++index)
    {
        auto& button = programButtons[static_cast<std::size_t>(index)];
        button.setLookAndFeel(&panelLookAndFeel);
        button.getProperties().set(juce::Identifier("grey"), true);
        button.setComponentID("program-" + juce::String(index + 1));
        button.onClick = [this, index]
        {
            const auto current = processor.getCurrentProgram();
            const auto requested = pro55::withProgramDigit(current, selectedDigit, index + 1);
            if (requested < processor.getNumPrograms())
                processor.setCurrentProgram(requested);
            refreshPresetControls();
        };
        addAndMakeVisible(button);
    }

    programDisplay.getProgram = [this] { return processor.getCurrentProgram(); };
    programDisplay.setProgram = [this](int index) { goToProgram(index); };
    addAndMakeVisible(programDisplay);
    programDisplay.setComponentID("program-digits");

    woodenPanel.onClick = [this] { aboutBox.setVisible(true); };
    addAndMakeVisible(woodenPanel);

    aboutBox.setLogo(juce::ImageFileFormat::loadFrom(BinaryData::about_logo_png, BinaryData::about_logo_pngSize));
    addChildComponent(aboutBox);

    setWantsKeyboardFocus(true);
    setSize(1143, 537);
    refreshPresetControls();
    startTimerHz(10);
}

Pro55AudioProcessorEditor::PanelLookAndFeel::PanelLookAndFeel()
{
#if PRO55_ALT_GUI
    blackKnobs = juce::ImageFileFormat::loadFrom(BinaryData::knob_black_png,
                                                  BinaryData::knob_black_pngSize);
#else
    blackKnobs = juce::ImageFileFormat::loadFrom(BinaryData::knob_black_legacy_png,
                                                  BinaryData::knob_black_legacy_pngSize);
#endif
#if PRO55_ALT_GUI
    metalKnobs = juce::ImageFileFormat::loadFrom(BinaryData::knob_metal_png,
                                                  BinaryData::knob_metal_pngSize);
#else
    metalKnobs = juce::ImageFileFormat::loadFrom(BinaryData::knob_metal_legacy_png, BinaryData::knob_metal_legacy_pngSize);
#endif
#if PRO55_ALT_GUI
    blackButton = juce::ImageFileFormat::loadFrom(BinaryData::button_black_png,
                                                   BinaryData::button_black_pngSize);
#else
    blackButton = juce::ImageFileFormat::loadFrom(BinaryData::button_black_legacy_png,
                                                   BinaryData::button_black_legacy_pngSize);
#endif
#if PRO55_ALT_GUI
    greyButton = juce::ImageFileFormat::loadFrom(BinaryData::button_grey_png,
                                                  BinaryData::button_grey_pngSize);
#else
    greyButton = juce::ImageFileFormat::loadFrom(BinaryData::button_grey_legacy_png,
                                                  BinaryData::button_grey_legacy_pngSize);
#endif
#if PRO55_ALT_GUI
    orangeButton = juce::ImageFileFormat::loadFrom(BinaryData::button_orange_png,
                                                    BinaryData::button_orange_pngSize);
#else
    orangeButton = juce::ImageFileFormat::loadFrom(BinaryData::button_orange_legacy_png,
                                                    BinaryData::button_orange_legacy_pngSize);
#endif
    wheels = juce::ImageFileFormat::loadFrom(BinaryData::wheel_png, BinaryData::wheel_pngSize);
    filterSelector = juce::ImageFileFormat::loadFrom(BinaryData::filter_selector_png,
                                                      BinaryData::filter_selector_pngSize);
}

void Pro55AudioProcessorEditor::FilterSelector::mouseDown(const juce::MouseEvent&)
{
    if (parameter != nullptr)
    {
        const auto current = parameter->convertFrom0to1(parameter->getValue());
        const auto next = current < 0.3f ? 0.5f : (current > 0.6f ? 0.0f : 1.0f);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(next));
        parameter->endChangeGesture();
    }
}

void Pro55AudioProcessorEditor::PanelLookAndFeel::drawLinearSlider(
    juce::Graphics& graphics, int x, int y, int width, int height, float,
    float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    if (slider.getProperties()["voices"])
        return;
    if (slider.getProperties()["filterSelector"])
    {
        constexpr int frameHeight = 56;
        constexpr int sourceWidth = 20;
        const auto frame = juce::jlimit(0, 2, juce::roundToInt(slider.getValue() * 2.0));
        const auto bounds = slider.getLocalBounds();
        const auto destinationWidth = juce::roundToInt(static_cast<float>(bounds.getWidth()) * 0.2f);
        graphics.drawImage(filterSelector, bounds.getX(), bounds.getY(),
                           destinationWidth, bounds.getHeight(),
                           0, frame * frameHeight, sourceWidth, frameHeight);
        return;
    }
    constexpr int frameHeight = 142;
    constexpr int frameCount = 64;
    const auto range = slider.getRange();
    const auto normalisedValue = range.getLength() > 0.0
        ? static_cast<float>((slider.getValue() - range.getStart()) / range.getLength())
        : 0.0f;
    const auto frame = juce::jlimit(0, frameCount - 1,
        juce::roundToInt(normalisedValue * static_cast<float>(frameCount - 1)));
    graphics.drawImage(wheels, x, y, width, height, 0, frame * frameHeight,
                       wheels.getWidth(), frameHeight);
}

void Pro55AudioProcessorEditor::PanelLookAndFeel::drawRotarySlider(
    juce::Graphics& graphics, int x, int y, int width, int height, float position,
    float, float, juce::Slider& slider)
{
    const auto& strip = slider.getProperties()["metal"] ? metalKnobs : blackKnobs;
    constexpr int frameCount = 128;
    const auto frameHeight = strip.getHeight() / frameCount;
    const auto frame = juce::jlimit(0, frameCount - 1,
        juce::roundToInt(position * static_cast<float>(frameCount - 1)));
    graphics.drawImage(strip, x, y, width, height, 0, frame * frameHeight,
                       strip.getWidth(), frameHeight);
}

void Pro55AudioProcessorEditor::PanelLookAndFeel::drawToggleButton(
    juce::Graphics& graphics, juce::ToggleButton& button, bool, bool shouldDrawButtonAsDown)
{
    const auto isOrange = static_cast<bool>(button.getProperties()["orange"]);
    const auto isGrey = static_cast<bool>(button.getProperties()["grey"]);

    // All switches use four 26x38 frames: off/raised, off/pressed, on/raised, on/pressed.
    // The normal off/raised state remains baked into the background so frame 1 need not be drawn.
    const auto& image = isOrange ? orangeButton : (isGrey ? greyButton : blackButton);
    const auto isOn = button.getToggleState();
    if (! isOn && ! shouldDrawButtonAsDown)
        return;

    constexpr int frameWidth = 26;
    constexpr int frameHeight = 38;
    const auto frame = isOn ? (shouldDrawButtonAsDown ? 3 : 2) : 1;
    const auto bounds = button.getLocalBounds();
    graphics.drawImage(image, bounds.getX(), bounds.getY(),
                       bounds.getWidth(), bounds.getHeight(),
                       0, frame * frameHeight, frameWidth, frameHeight);
}

Pro55AudioProcessorEditor::AboutBox::AboutBox()
{
    for (auto* link : { &cmajorLink, &sourceLink, &truexLink })
    {
        link->setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xff0645ad));
        link->setJustificationType(juce::Justification::centredLeft);
        link->setTooltip(link->getURL().toString(false));
        addAndMakeVisible(*link);
    }
}

juce::Rectangle<float> Pro55AudioProcessorEditor::AboutBox::getTextBounds() const
{
    const auto bounds = getLocalBounds().toFloat();
    const auto card = bounds.withSizeKeepingCentre(bounds.getWidth() * 0.56f, bounds.getHeight() * 0.55f);
    auto content = card.reduced(card.getWidth() * 0.05f, card.getHeight() * 0.08f);
    content.removeFromLeft(content.getWidth() * 0.3f);
    content.removeFromLeft(content.getWidth() * 0.06f);
    return content;
}

void Pro55AudioProcessorEditor::AboutBox::resized()
{
    const auto textBounds = getTextBounds();
    const auto fontHeight = juce::jmax(10.0f, getHeight() * 0.55f * 0.055f);
    const auto linkHeight = juce::roundToInt(fontHeight * 1.35f);
    const juce::Font linkFont { juce::FontOptions(fontHeight) };

    const auto placeLink = [&] (juce::HyperlinkButton& link, float verticalPosition)
    {
        link.setFont(linkFont, false, juce::Justification::centredLeft);
        link.setBounds(juce::roundToInt(textBounds.getX()),
                       juce::roundToInt(textBounds.getY() + textBounds.getHeight() * verticalPosition),
                       juce::roundToInt(textBounds.getWidth()), linkHeight);
        link.changeWidthToFitText();
    };

    placeLink(cmajorLink, 0.36f);
    placeLink(sourceLink, 0.76f);
    placeLink(truexLink, 0.90f);
}
void Pro55AudioProcessorEditor::AboutBox::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colours::black.withAlpha(0.53f));

    const auto bounds = getLocalBounds().toFloat();
    const auto card = bounds.withSizeKeepingCentre(bounds.getWidth() * 0.56f, bounds.getHeight() * 0.55f);
    graphics.setColour(juce::Colour(0xffeeeeee));
    graphics.fillRoundedRectangle(card, 6.0f);
    graphics.setColour(juce::Colours::black.withAlpha(0.4f));
    graphics.drawRoundedRectangle(card, 6.0f, 1.5f);

    auto content = card.reduced(card.getWidth() * 0.05f, card.getHeight() * 0.08f);
    const auto logoArea = content.removeFromLeft(content.getWidth() * 0.3f).reduced(4.0f);
    if (logo.isValid())
    {
        // The source file's fully-transparent pixels carry leftover, non-black RGB (a
        // vignette baked in by whatever exported it), invisible at full size but mixed
        // into the result once this gets downscaled into a small logoArea, washing the
        // logo out to grey. Using the alpha channel purely as a mask filled with the
        // current colour ignores that stray RGB entirely and always renders pure black.
        graphics.setColour(juce::Colours::black);
        graphics.drawImage(logo, logoArea, juce::RectanglePlacement::centred, true);
    }
    content = getTextBounds();

    graphics.setColour(juce::Colours::black);
    graphics.setFont(juce::FontOptions(juce::jmax(10.0f, card.getHeight() * 0.055f)));
    const auto drawBlock = [&] (const juce::String& text, float top, float height, int maximumLines)
    {
        auto area = content.withY(content.getY() + content.getHeight() * top)
                           .withHeight(content.getHeight() * height)
                           .toNearestInt();
        graphics.drawFittedText(text, area, juce::Justification::topLeft, maximumLines);
    };

    drawBlock(juce::String::fromUTF8(
                  "Pro-55 \xe2\x80\x94 a TrueX Audio instrument based on Cmajor Pro-54, itself a port of the "
                  "Native Instruments Pro-53 synthesiser."),
              0.0f, 0.24f, 3);
    drawBlock(juce::String::fromUTF8(
                  "DSP ported to Cmajor by Cmajor Software Ltd \xe2\x80\x94"),
              0.25f, 0.11f, 2);
    drawBlock("Original Pro-53 by Native Instruments.", 0.45f, 0.08f, 1);
    drawBlock("Pro-55 native code is licensed under the AGPLv3. Third-party components retain "
              "their own terms. Source, licences, and notices:", 0.54f, 0.21f, 3);
}

void Pro55AudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colours::black);
    graphics.drawImage(background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    const auto sx = static_cast<float>(getWidth()) / 1143.0f;
    const auto sy = static_cast<float>(getHeight()) / 537.0f;
    const auto program = juce::jlimit(0, 511, processor.getCurrentProgram());
    const std::array digits { program / 64 + 1, (program / 8) % 8 + 1, program % 8 + 1 };
    for (int index = 0; index < 3; ++index)
    {
        const auto digitBounds = layoutRect(246.0f + 10.0f * index, 166.0f, 10.0f, 14.0f, sx, sy);
        graphics.drawImage(programDigits, digitBounds.getX(), digitBounds.getY(),
            digitBounds.getWidth(), digitBounds.getHeight(),
            0, (digits[static_cast<std::size_t>(index)] - 1) * 28, 20, 28);
    }

    const auto voices = juce::jlimit(1, 32,
        juce::roundToInt(processor.parameters.getRawParameterValue("ActiveVoices")->load()));
    // voice_digits.png is 40x896: 32 frames of 40x28, one per voice count, each frame
    // carrying its own bezel. It must be drawn at true size or the frame stride
    // drifts by a pixel per step and neighbouring frames bleed into the window.
    const auto voicesBounds = layoutRect(250.0f, 119.0f, 20.0f, 14.0f, sx, sy);
    graphics.drawImage(voiceDigits, voicesBounds.getX(), voicesBounds.getY(),
        voicesBounds.getWidth(), voicesBounds.getHeight(),
        0, (voices - 1) * 28, 40, 28);

    if (midiLightTicks > 0)
        graphics.drawImage(midiLight, layoutRect(723.0f, 168.0f, 9.0f, 9.0f, sx, sy).toFloat());

    // In the New/New AA filter modes the FILTER section's HPF switch sonically acts as a
    // band-pass filter instead, so a small BPF placard lights up beside it to say so. Only
    // Old (FilterVersion == 0.0, the bottom position lighting the bottom red dot) leaves
    // the switch as a true HPF; 0.5 is New and 1.0 is New AA, both showing BPF.
    const auto filterVersionValue = processor.parameters.getRawParameterValue("FilterVersion")->load();
    if (filterVersionValue > 0.1f)
        graphics.drawImage(bpfIndicator, layoutRect(573.0f, 47.0f, 17.0f, 9.0f, sx, sy).toFloat());
}

void Pro55AudioProcessorEditor::resized()
{
    if (controls.size() != static_cast<int>(std::size(controlSpecs)))
        return;

    const auto sx = static_cast<float>(getWidth()) / 1143.0f;
    const auto sy = static_cast<float>(getHeight()) / 537.0f;
    for (std::size_t i = 0; i < std::size(controlSpecs); ++i)
    {
        const auto& spec = controlSpecs[i];
        const auto width = spec.button ? 13.0f : 25.0f;
        const auto height = spec.button ? 19.0f : 25.0f;
        controls[static_cast<int>(i)]->setBounds(
            layoutRect(static_cast<float>(spec.x), static_cast<float>(spec.y), width, height, sx, sy));
    }

    const auto scaled = [sx, sy](float x, float y, float width, float height)
    {
        return layoutRect(x, y, width, height, sx, sy);
    };
    saveButton.setBounds(scaled(174, 164, 13, 19));
    saveAsButton.setBounds(scaled(202, 164, 13, 19));
    bankButton.setBounds(scaled(222, 164, 13, 19));
    programDisplay.setBounds(scaled(246, 166, 30, 14));
    presetName.setBounds(scaled(454, 166, 126, 13));
    // The font is set once at construction, so it has to be rescaled with the panel
    // or the name stays at its smallest size while the artwork around it grows.
    // applyFontToAllText is what reaches text that is already in the editor.
    presetName.applyFontToAllText(juce::Font(
        juce::FontOptions(static_cast<float>(presetName.getHeight()) * 0.75f)
            .withName(juce::Font::getDefaultMonospacedFontName())));
    presetSelector.setBounds(scaled(454, 164, 128, 17));
    for (int index = 0; index < static_cast<int>(programButtons.size()); ++index)
        programButtons[static_cast<std::size_t>(index)].setBounds(scaled(286.0f + 20.0f * index, 164, 13, 19));
    pitchWheel.setBounds(scaled(27, 258, 10, 71));
    modWheel.setBounds(scaled(55, 258, 10, 71));
    filterVersion.setBounds(scaled(24, 162, 50, 28));
    activeVoices.setBounds(scaled(250, 119, 20, 14));
#if PRO55_ALT_GUI
    // The hand-edited key map is registered to the complete 1498x704 panel, so the
    // transparent keyboard component uses the same full-panel coordinate space.
    keyboard.setBounds(getLocalBounds());
    keyboard.setKeyWidth(static_cast<float>(keyboard.getWidth()) / 36.0f);
#else
    keyboard.setBounds(scaled(82, 249, 668, 102));
    keyboard.setKeyWidth(static_cast<float>(keyboard.getWidth()) / 36.0f);
#endif
    // The wooden banner strip between the control panel and the keyboard; 2% margins
    // either side matches the original CSS layout it was ported from.
    woodenPanel.setBounds(scaled(762.0f * 0.02f, 208, 762.0f * 0.96f, 33));
    aboutBox.setBounds(getLocalBounds());
}

void Pro55AudioProcessorEditor::refreshPresetControls()
{
    const auto current = processor.getCurrentProgram();
    const auto revision = processor.getProgramRevision();
    if (displayedRevision != revision || displayedProgram != current)
    {
        displayedRevision = revision;
        displayedProgram = current;
        presetSelector.clear(juce::dontSendNotification);
        for (int index = 0; index < processor.getNumPrograms(); ++index)
            presetSelector.addItem(juce::String(pro55::programAddress(index)) + "  "
                                   + processor.getProgramName(index), index + 1);
        presetSelector.setSelectedItemIndex(current, juce::dontSendNotification);
    }
    if (presetName.isReadOnly())
        presetName.setText(processor.getCurrentSoundName(), false);
    presetName.setRenameEnabled(processor.isRecordArmed());
    saveButton.setToggleState(processor.isRecordArmed(), juce::dontSendNotification);
    saveAsButton.setToggleState(selectedDigit == 0, juce::dontSendNotification);
    bankButton.setToggleState(selectedDigit == 1, juce::dontSendNotification);
    const auto stride = selectedDigit == 0 ? 64 : selectedDigit == 1 ? 8 : 1;
    for (int index = 0; index < static_cast<int>(programButtons.size()); ++index)
        programButtons[static_cast<std::size_t>(index)].setToggleState(index == (current / stride) % 8,
                                                                       juce::dontSendNotification);
}

void Pro55AudioProcessorEditor::recordPressed(bool overwriteChangedBank)
{
    if (recordConfirmationOpen) return;
    presetName.finishRename();
    const auto result = processor.toggleRecord(presetName.getText(), overwriteChangedBank);
    refreshPresetControls();
    if (result.failed() && processor.presets.hasSaveConflict())
    {
        recordConfirmationOpen = true;
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
            "Shared bank changed", result.getErrorMessage(), "Replace bank", "Cancel", this,
            juce::ModalCallbackFunction::create(
                [safe = juce::Component::SafePointer<Pro55AudioProcessorEditor>(this)](int choice)
                {
                    if (safe == nullptr) return;
                    safe->recordConfirmationOpen = false;
                    if (choice != 0) safe->recordPressed(true);
                }));
    }
    else showPresetError(result);
}

void Pro55AudioProcessorEditor::timerCallback()
{
    const auto warning = processor.takeStartupWarning();
    if (warning.isNotEmpty()) showPresetError(juce::Result::fail(warning));
    if (processor.consumeMidiActivity())
        midiLightTicks = 2;
    else if (midiLightTicks > 0)
        --midiLightTicks;
    refreshPresetControls();
    repaint();
}

void Pro55AudioProcessorEditor::goToProgram(int index)
{
    const auto count = processor.getNumPrograms();
    if (count <= 0)
        return;

    const auto next = juce::jlimit(0, count - 1, index);
    if (next == processor.getCurrentProgram())
        return;

    processor.setCurrentProgram(next);
    refreshPresetControls();
}

bool Pro55AudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::F2Key) { presetName.beginRename(); return true; }
    if (! presetName.isReadOnly() && presetName.hasKeyboardFocus(true)) return false;
    if (key == juce::KeyPress::leftKey)
    {
        goToProgram(processor.getCurrentProgram() - 1);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        goToProgram(processor.getCurrentProgram() + 1);
        return true;
    }
    return false;
}

void Pro55AudioProcessorEditor::showPresetError(const juce::Result& result)
{
    if (result.failed())
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Pro-55 preset", result.getErrorMessage());
}

void Pro55AudioProcessorEditor::addControl(const ControlSpec& spec)
{
    if (spec.button)
    {
        auto* button = new juce::ToggleButton();
        controls.add(button);
        button->setLookAndFeel(&panelLookAndFeel);
        button->getProperties().set(juce::Identifier("grey"), juce::String(spec.id) == "TestTone");
        button->setTooltip(processor.parameters.getParameter(spec.id)->getName(64));
        addAndMakeVisible(button);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.parameters, spec.id, *button));
        return;
    }

    auto* slider = new juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
    controls.add(slider);
    slider->setLookAndFeel(&panelLookAndFeel);
    slider->getProperties().set(juce::Identifier("metal"), juce::String(spec.id) == "MasterTune"
                                                            || juce::String(spec.id) == "Volume");
    slider->setTooltip(processor.parameters.getParameter(spec.id)->getName(64));
    addAndMakeVisible(slider);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, spec.id, *slider));
}
