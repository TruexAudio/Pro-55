#pragma once

#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

class Pro55AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit Pro55AudioProcessorEditor(Pro55AudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

    struct ControlSpec
    {
        const char* id;
        int x;
        int y;
        bool button;
    };

private:
    class SpringSlider final : public juce::Slider
    {
    public:
        SpringSlider() : juce::Slider(juce::Slider::LinearVertical, juce::Slider::NoTextBox) {}
        void mouseUp(const juce::MouseEvent& event) override
        {
            juce::Slider::mouseUp(event);
            setValue(50.0, juce::sendNotificationSync);
        }
    };

    class FilterSelector final : public juce::Slider
    {
    public:
        FilterSelector() : juce::Slider(juce::Slider::LinearVertical, juce::Slider::NoTextBox) {}
        void setParameter(juce::RangedAudioParameter& newParameter) { parameter = &newParameter; }
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override {}
        void mouseDoubleClick(const juce::MouseEvent&) override {}

    private:
        juce::RangedAudioParameter* parameter = nullptr;
    };

    class PanelKeyboard final : public juce::MidiKeyboardComponent
    {
    public:
        explicit PanelKeyboard(juce::MidiKeyboardState&);
        void setKeyMap(juce::Image);
        void paint(juce::Graphics&) override;
        bool hitTest(int x, int y) override;
        void mouseMove(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseEnter(const juce::MouseEvent&) override;
        void mouseExit(const juce::MouseEvent&) override;

    private:
#if PRO55_ALT_GUI
        struct KeyMask
        {
            juce::Image image;
            juce::Rectangle<int> panelBounds;
        };

        static constexpr int firstMappedNote = 36;
        static constexpr int mappedNoteCount = 61;
        int keyIndexForColour(juce::Colour) const;
        int noteAt(juce::Point<int>) const;
        float velocityAt(int note, juce::Point<int>) const;
        void updateMouseNote(const juce::MouseEvent&, bool isDown);

        juce::MidiKeyboardState& keyboardState;
        juce::Image keyMap;
        std::array<KeyMask, mappedNoteCount> keyMasks;
        std::vector<signed char> noteIndices;
        int hoveredNote = -1;
        int mouseNote = -1;
#endif
    };

    // Single-click browses; F2/right-click renames. Armed clicks rename directly.
    class PresetNameEditor final : public juce::TextEditor
    {
    public:
        std::function<void()> onBrowseRequested;
        std::function<void()> onRenameCommitted;
        void setRenameEnabled(bool value) { renameEnabled = value; }
        void beginRename()
        {
            if (! isReadOnly()) return;
            originalText = getText();
            setReadOnly(false);
            grabKeyboardFocus();
            setCaretPosition(getText().length());
        }
        void finishRename(bool cancel = false)
        {
            if (isReadOnly()) return;
            if (cancel) setText(originalText, true);
            if (onRenameCommitted) onRenameCommitted();
            setReadOnly(true);
            giveAwayKeyboardFocus();
        }
        void mouseDown(const juce::MouseEvent& event) override
        {
            if (! isReadOnly()) { juce::TextEditor::mouseDown(event); return; }
            if (event.mods.isPopupMenu())
            {
                juce::PopupMenu menu;
                menu.addItem(1, "Rename patch (F2)");
                menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                    [safe = juce::Component::SafePointer<PresetNameEditor>(this)](int choice)
                    { if (safe != nullptr && choice == 1) safe->beginRename(); });
            }
            else if (renameEnabled) beginRename();
            else if (onBrowseRequested) onBrowseRequested();
        }
        void mouseDoubleClick(const juce::MouseEvent& event) override
        {
            if (! isReadOnly()) juce::TextEditor::mouseDoubleClick(event);
        }
        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::F2Key) { beginRename(); return true; }
            return ! isReadOnly() && juce::TextEditor::keyPressed(key);
        }
        void focusLost(FocusChangeType cause) override
        {
            juce::TextEditor::focusLost(cause);
            if (! isReadOnly() && onRenameCommitted) onRenameCommitted();
            setReadOnly(true);
        }
    private:
        bool renameEnabled = false;
        juce::String originalText;
    };

    // Three independent digit hit regions coincide with the painted digits.
    class ProgramDisplay final : public juce::Component
    {
    public:
        std::function<int()> getProgram;
        std::function<void(int)> setProgram;
        ProgramDisplay() { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); }
        void mouseDown(const juce::MouseEvent& event) override
        {
            dragStartProgram = getProgram ? getProgram() : 0;
            digit = juce::jlimit(0, 2, event.x * 3 / juce::jmax(1, getWidth()));
        }
        void mouseDrag(const juce::MouseEvent& event) override
        {
            if (! setProgram) return;
            const auto steps = -event.getDistanceFromDragStartY() / juce::jmax(6, getHeight() / 3);
            const auto stride = digit == 0 ? 64 : digit == 1 ? 8 : 1;
            const auto value = juce::jlimit(1, 8, (dragStartProgram / stride) % 8 + 1 + steps);
            setProgram(pro55::withProgramDigit(dragStartProgram, digit, value));
        }
    private:
        int dragStartProgram = 0, digit = 2;
    };

    // Invisible hit area over the wooden banner between the control panel and the
    // keyboard. A plain Button (rather than a raw Component + mouseDown) gets normal
    // click semantics for free: a drag that leaves the area before release is not a
    // click, matching how every other control on this panel already behaves.
    class WoodenPanelButton final : public juce::Button
    {
    public:
        WoodenPanelButton() : juce::Button({})
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }

    private:
        void paintButton(juce::Graphics&, bool, bool) override {}
    };

    // The credits screen the wooden panel reveals. Clicking anywhere except one of
    // its links dismisses it; the links launch in the system's default browser.
    class AboutBox final : public juce::Component
    {
    public:
        AboutBox();
        void setLogo(juce::Image image) { logo = std::move(image); }
        void paint(juce::Graphics&) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent&) override { setVisible(false); }

    private:
        juce::Rectangle<float> getTextBounds() const;

        juce::Image logo;
        juce::HyperlinkButton cmajorLink { "https://cmajor.dev", juce::URL("https://cmajor.dev") };
        juce::HyperlinkButton sourceLink { "https://github.com/TruexAudio/Pro-55",
                                           juce::URL("https://github.com/TruexAudio/Pro-55") };
        juce::HyperlinkButton truexLink { "https://truexaudio.com", juce::URL("https://truexaudio.com") };
    };

    class PanelLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        PanelLookAndFeel();
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider&) override;
        void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider::SliderStyle, juce::Slider&) override;

    private:
        juce::Image blackKnobs;
        juce::Image metalKnobs;
        juce::Image blackButton;
        juce::Image greyButton;
        juce::Image orangeButton;
        juce::Image wheels;
        juce::Image filterSelector;
    };

    void addControl(const ControlSpec&);
    void refreshPresetControls();
    void timerCallback() override;
    void showPresetError(const juce::Result&);
    void goToProgram(int index);
    void recordPressed(bool overwriteChangedBank = false);
    bool keyPressed(const juce::KeyPress&) override;

    Pro55AudioProcessor& processor;
    PanelLookAndFeel panelLookAndFeel;
    juce::Image background;
    juce::OwnedArray<juce::Component> controls;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetSelector;
    ProgramDisplay programDisplay;
    PresetNameEditor presetName;
    juce::ToggleButton saveButton;
    juce::ToggleButton saveAsButton;
    juce::ToggleButton bankButton;
    std::array<juce::ToggleButton, 8> programButtons;
    int displayedProgram = -1;
    unsigned displayedRevision = ~0u;
    int selectedDigit = 2;
    bool recordConfirmationOpen = false;
    SpringSlider pitchWheel;
    juce::Slider modWheel { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider activeVoices { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    FilterSelector filterVersion;
    PanelKeyboard keyboard;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> specialAttachments;
    juce::Image programDigits;
    juce::Image voiceDigits;
    juce::Image midiLight;
    juce::Image bpfIndicator;
    int midiLightTicks = 0;
    WoodenPanelButton woodenPanel;
    AboutBox aboutBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pro55AudioProcessorEditor)
};
