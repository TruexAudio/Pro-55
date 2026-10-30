#include "PresetLibrary.h"
#include <array>
#include <cmath>

namespace pro55
{
PresetLibrary::PresetLibrary(juce::File rootDirectory) : root(std::move(rootDirectory)) {}

void PresetLibrary::setFactoryBank(Bank bank)
{
    bank.factory = true;
    factoryBank = std::move(bank);
    workingBank = factoryBank;
}

juce::File PresetLibrary::defaultRootDirectory()
{
#if JUCE_LINUX || JUCE_BSD
    const auto config = juce::SystemStats::getEnvironmentVariable("XDG_CONFIG_HOME", {});
    auto base = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".config");
    if (juce::File::isAbsolutePath(config)) base = juce::File(config);
#else
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    base = base.getChildFile("Application Support");
#endif
#endif
    return base.getChildFile("Truex Audio").getChildFile("Pro55");
}

juce::Result PresetLibrary::load()
{
    workingBank = factoryBank;
    saveConflict = damagedBank = false;
    const auto file = userBankFile();
    loadedFileExisted = file.exists();
    loadedContents = file.loadFileAsString();
    if (! loadedFileExisted) return juce::Result::ok();
    juce::var json;
    auto status = juce::JSON::parse(loadedContents, json);
    if (status.wasOk()) status = fromUserBankJson(json, factoryBank, workingBank);
    if (status.failed())
    {
        damagedBank = true;
        return juce::Result::fail("Using the factory bank because the user bank could not be read. "
            "The original file is protected. Move it aside before saving a new bank.\n\n"
            + file.getFullPathName() + "\n" + status.getErrorMessage());
    }
    return juce::Result::ok();
}

void PresetLibrary::renamePreset(std::size_t slot, const juce::String& name)
{
    if (slot < workingBank.presets.size() && name.trim().isNotEmpty())
        workingBank.presets[slot].name = name.trim();
}

juce::Result PresetLibrary::savePreset(std::size_t slot, const Preset& preset, bool overwriteChangedBank)
{
    saveConflict = false;
    if (slot >= workingBank.presets.size() || workingBank.presets.size() != programCount)
        return juce::Result::fail("Preset slot is outside the 512-program bank");
    if (preset.name.trim().isEmpty()) return juce::Result::fail("Enter a name for the preset");
    const auto file = userBankFile();
    juce::InterProcessLock lock("Pro55UserBank-" + juce::String::toHexString(file.getFullPathName().hashCode64()));
    if (! lock.enter(100))
        return juce::Result::fail("Another Pro-55 instance is saving. Please try Record again.");
    struct Unlock { juce::InterProcessLock& lock; ~Unlock() { lock.exit(); } } unlock { lock };
    if (damagedBank && file.exists())
        return juce::Result::fail("The unreadable user bank is protected. Move it aside before "
            "saving; your captured sound is still held.\n" + file.getFullPathName());
    const auto contentsNow = file.loadFileAsString();
    if (! overwriteChangedBank && ! damagedBank
        && (file.exists() != loadedFileExisted || contentsNow != loadedContents))
    {
        saveConflict = true;
        return juce::Result::fail("The shared bank has changed since this instance loaded it. "
            "Replace it with this instance's complete bank? Changes saved by another instance "
            "will be replaced. Cancel keeps your captured sound without saving.");
    }
    auto candidate = workingBank;
    candidate.factory = false;
    candidate.presets[slot] = preset;
    const auto contents = juce::JSON::toString(toUserBankJson(candidate), true);
    if (root.createDirectory().failed())
        return juce::Result::fail("Could not create preset folder: " + root.getFullPathName());
    juce::TemporaryFile temporary(file);
    if (! temporary.getFile().replaceWithText(contents) || ! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail("Could not save the bank. Your captured sound is still held.\n" + file.getFullPathName());
    workingBank = std::move(candidate);
    loadedContents = contents;
    loadedFileExisted = true;
    damagedBank = false;
    return juce::Result::ok();
}

juce::var PresetLibrary::toUserBankJson(const Bank& bank)
{
    juce::Array<juce::var> entries;
    for (std::size_t i = 0; i < bank.presets.size(); ++i)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        for (const auto& [id, value] : bank.presets[i].parameters) object->setProperty(id, value);
        object->setProperty("id", programAddress(static_cast<int>(i)));
        object->setProperty("PatchName", bank.presets[i].name);
        entries.add(juce::var(object.release()));
    }
    return entries;
}

juce::Result PresetLibrary::fromUserBankJson(const juce::var& json, const Bank& defaults, Bank& result)
{
    const auto* entries = json.getArray();
    if (entries == nullptr || entries->size() != programCount || defaults.presets.size() != programCount)
        return juce::Result::fail("UserBank.json must contain 512 patches");
    auto parsed = defaults;
    parsed.factory = false;
    parsed.id = "user";
    parsed.name = "User";
    std::array<bool, programCount> seen {};
    for (const auto& entry : *entries)
    {
        const auto* object = entry.getDynamicObject();
        if (object == nullptr) return juce::Result::fail("Patch must be a JSON object");
        const auto addressValue = object->getProperty("id");
        if (! addressValue.isInt() && ! addressValue.isInt64()) return juce::Result::fail("Patch address must be an integer");
        if (static_cast<juce::int64>(addressValue) < 111 || static_cast<juce::int64>(addressValue) > 888)
            return juce::Result::fail("Patch address is outside 111-888");
        const auto address = static_cast<int>(addressValue);
        const int a = address / 100, b = (address / 10) % 10, c = address % 10;
        if (a < 1 || a > 8 || b < 1 || b > 8 || c < 1 || c > 8)
            return juce::Result::fail("Patch address must have three digits from 1 to 8");
        const auto index = static_cast<std::size_t>((a - 1) * 64 + (b - 1) * 8 + c - 1);
        if (seen[index]) return juce::Result::fail("Duplicate patch address");
        seen[index] = true;
        const auto name = object->getProperty("PatchName");
        if (! name.isString() || name.toString().trim().isEmpty()) return juce::Result::fail("Patch name is missing");
        auto& preset = parsed.presets[index];
        preset.id = juce::String(address);
        preset.name = name.toString();
        int valueCount = 0;
        for (const auto& property : object->getProperties())
        {
            if (property.name == juce::Identifier("id") || property.name == juce::Identifier("PatchName")) continue;
            const auto& value = property.value;
            if ((! value.isDouble() && ! value.isInt() && ! value.isInt64() && ! value.isBool())
                || ! std::isfinite(static_cast<double>(value)) || ! std::isfinite(static_cast<float>(value)))
                return juce::Result::fail("Patch parameters must be finite numbers");
            preset.parameters[property.name.toString()] = static_cast<float>(value);
            ++valueCount;
        }
        if (valueCount == 0) return juce::Result::fail("Patch has no parameters");
    }
    result = std::move(parsed);
    return juce::Result::ok();
}
}
