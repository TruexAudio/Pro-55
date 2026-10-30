#include "presets/PresetLibrary.h"
#include <iostream>
#include <stdexcept>

static void require(bool value, const char* message)
{
    if (! value) throw std::runtime_error(message);
}

static pro55::Bank factoryBank()
{
    pro55::Bank bank { "factory", "Factory", true, {} };
    for (int i = 0; i < 512; ++i)
        bank.presets.push_back({ juce::String(i), "Factory " + juce::String(i),
                                { { "Volume", 100.0f }, { "FilterVersion", 0.0f } } });
    return bank;
}

int main()
{
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("Pro55-BankTest-" + juce::Uuid().toString());
    int code = 0;
    try
    {
        const auto factory = factoryBank();
        require(pro55::programAddress(0) == 111 && pro55::programAddress(511) == 888, "Address endpoints");
        for (int i = 0; i < 512; ++i)
            for (int digit = 0; digit < 3; ++digit)
                for (int value = 1; value <= 8; ++value)
                {
                    const int stride = digit == 0 ? 64 : digit == 1 ? 8 : 1;
                    const int changed = pro55::withProgramDigit(i, digit, value);
                    require(changed >= 0 && changed < 512, "Digit bounds");
                    require((changed / stride) % 8 == value - 1, "Digit selection");
                    require(changed - i == (value - 1 - (i / stride) % 8) * stride, "Other digits preserved");
                }

        pro55::PresetLibrary first(root), second(root);
        first.setFactoryBank(factory);
        second.setFactoryBank(factory);
        require(first.load().wasOk() && second.load().wasOk(), "Factory fallback");
        first.renamePreset(0, "RAM only");
        require(! root.exists(), "Loading/renaming must not create a folder");
        auto patch = factory.presets[0];
        patch.name = "Stored";
        patch.parameters["Volume"] = 42.0f;
        require(first.savePreset(511, patch).wasOk(), "First Record save");
        require(first.userBankFile().existsAsFile(), "Folder and file created automatically");
        require(first.activeBank().presets[0].name == "RAM only", "Pending rename retained");
        require(second.savePreset(10, patch).failed() && second.hasSaveConflict(), "Stale instance must warn");
        require(second.activeBank().presets[10].name == factory.presets[10].name, "Failed save is transactional");
        require(second.savePreset(10, patch, true).wasOk(), "Explicit overwrite confirmation");

        pro55::PresetLibrary reopened(root);
        reopened.setFactoryBank(factory);
        require(reopened.load().wasOk(), "Read original-compatible JSON");
        require(reopened.activeBank().presets[10].parameters.at("Volume") == 42.0f, "Parameter round trip");
        const auto contents = reopened.userBankFile().loadFileAsString();
        require(juce::JSON::parse(contents).isArray(), "Original flat array format");
        auto json = pro55::PresetLibrary::toUserBankJson(factory);
        pro55::Bank parsed;
        json.getArray()->getReference(0).getDynamicObject()->removeProperty("FilterVersion");
        require(pro55::PresetLibrary::fromUserBankJson(json, factory, parsed).wasOk(), "Older bank missing new parameter");
        require(parsed.presets[0].parameters.at("FilterVersion") == 0.0f, "Missing parameter defaults");
        json.getArray()->getReference(0).getDynamicObject()->setProperty("id", 112);
        require(pro55::PresetLibrary::fromUserBankJson(json, factory, parsed).failed(), "Duplicate address rejected");
        json = pro55::PresetLibrary::toUserBankJson(factory);
        json.getArray()->getReference(0).getDynamicObject()->setProperty("id", 109);
        require(pro55::PresetLibrary::fromUserBankJson(json, factory, parsed).failed(), "Invalid address rejected");
        json = pro55::PresetLibrary::toUserBankJson(factory);
        json.getArray()->getReference(0).getDynamicObject()->setProperty("Volume", "bad");
        require(pro55::PresetLibrary::fromUserBankJson(json, factory, parsed).failed(), "Non-numeric value rejected");

        require(reopened.userBankFile().replaceWithText("damaged test bank"), "Write damaged fixture");
        require(reopened.load().failed(), "Damaged bank reported");
        require(reopened.activeBank().presets[10].name == factory.presets[10].name, "Damaged bank factory fallback");
        require(reopened.savePreset(0, patch, true).failed(), "Damaged file protected even with confirmation");
        require(reopened.userBankFile().loadFileAsString() == "damaged test bank", "Damaged file untouched");
        require(reopened.userBankFile().moveFileTo(root.getChildFile("damaged-backup.json")), "Move damaged test fixture aside");
        require(reopened.savePreset(0, patch).wasOk(), "Save after damaged file moved aside");
        const auto blockedRoot = root.getChildFile("not-a-folder");
        require(blockedRoot.replaceWithText("block"), "Create failed-write fixture");
        pro55::PresetLibrary blocked(blockedRoot);
        blocked.setFactoryBank(factory);
        require(blocked.load().wasOk(), "Read-only startup no mkdir");
        require(blocked.savePreset(0, patch).failed(), "Write failure reported");
        require(blocked.activeBank().presets[0].name == factory.presets[0].name, "Write failure leaves bank unchanged");
        std::cout << "All bank/address tests passed\n";
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; code = 1; }
    // Only our uniquely named temporary test directory may be removed.
    if (root.getParentDirectory() == juce::File::getSpecialLocation(juce::File::tempDirectory)
        && root.getFileName().startsWith("Pro55-BankTest-")) root.deleteRecursively();
    return code;
}
