#include "PresetTypes.h"

namespace pro55
{
juce::var Preset::toJson() const
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("id", id);
    object->setProperty("name", name);

    auto values = std::make_unique<juce::DynamicObject>();
    for (const auto& [parameter, value] : parameters)
        values->setProperty(parameter, value);

    object->setProperty("parameters", juce::var(values.release()));
    return juce::var(object.release());
}

juce::Result Preset::fromJson(const juce::var& value, Preset& result)
{
    auto* object = value.getDynamicObject();
    if (object == nullptr)
        return juce::Result::fail("Preset must be a JSON object");

    auto* values = object->getProperty("parameters").getDynamicObject();
    if (values == nullptr)
        return juce::Result::fail("Preset is missing its parameter object");

    Preset parsed;
    parsed.id = object->getProperty("id").toString();
    parsed.name = object->getProperty("name").toString();
    if (parsed.id.isEmpty() || parsed.name.isEmpty())
        return juce::Result::fail("Preset id and name cannot be empty");

    for (const auto& property : values->getProperties())
    {
        if (! property.value.isDouble() && ! property.value.isInt() && ! property.value.isInt64())
            return juce::Result::fail("Preset parameter values must be numeric");

        parsed.parameters[property.name.toString()] = static_cast<float>(property.value);
    }

    result = std::move(parsed);
    return juce::Result::ok();
}

juce::var Bank::toJson() const
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("format", "pro55-bank");
    object->setProperty("version", currentFormatVersion);
    object->setProperty("id", id);
    object->setProperty("name", name);
    object->setProperty("factory", factory);

    juce::Array<juce::var> entries;
    for (const auto& preset : presets)
        entries.add(preset.toJson());
    object->setProperty("presets", entries);
    return juce::var(object.release());
}

juce::Result Bank::fromJson(const juce::var& value, Bank& result)
{
    auto* object = value.getDynamicObject();
    if (object == nullptr || object->getProperty("format").toString() != "pro55-bank")
        return juce::Result::fail("Not a Pro-55 bank");

    if (static_cast<int>(object->getProperty("version")) != currentFormatVersion)
        return juce::Result::fail("Unsupported Pro-55 bank version");

    auto* entries = object->getProperty("presets").getArray();
    if (entries == nullptr)
        return juce::Result::fail("Bank is missing its preset list");

    Bank parsed;
    parsed.id = object->getProperty("id").toString();
    parsed.name = object->getProperty("name").toString();
    parsed.factory = static_cast<bool>(object->getProperty("factory"));
    if (parsed.id.isEmpty() || parsed.name.isEmpty())
        return juce::Result::fail("Bank id and name cannot be empty");

    for (const auto& entry : *entries)
    {
        Preset preset;
        if (auto status = Preset::fromJson(entry, preset); status.failed())
            return status;
        parsed.presets.push_back(std::move(preset));
    }

    result = std::move(parsed);
    return juce::Result::ok();
}
}
