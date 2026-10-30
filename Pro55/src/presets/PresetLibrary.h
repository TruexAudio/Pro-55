#pragma once
#include "PresetTypes.h"
#include "ProgramAddress.h"

namespace pro55
{
// One per-user bank. Loading and renaming never write to disk.
class PresetLibrary
{
public:
    explicit PresetLibrary(juce::File rootDirectory);
    void setFactoryBank(Bank);
    const Bank& activeBank() const noexcept { return workingBank; }
    juce::Result load();
    juce::Result savePreset(std::size_t, const Preset&, bool overwriteChangedBank = false);
    void renamePreset(std::size_t, const juce::String&);
    bool hasSaveConflict() const noexcept { return saveConflict; }
    juce::File userBankFile() const { return root.getChildFile("UserBank.json"); }
    static juce::File defaultRootDirectory();
    static juce::var toUserBankJson(const Bank&);
    static juce::Result fromUserBankJson(const juce::var&, const Bank& defaults, Bank&);
private:
    juce::File root;
    Bank factoryBank, workingBank;
    juce::String loadedContents;
    bool loadedFileExisted = false, damagedBank = false, saveConflict = false;
};
}
