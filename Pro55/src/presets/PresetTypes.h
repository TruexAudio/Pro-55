#pragma once

#include <juce_core/juce_core.h>
#include <map>

namespace pro55
{
using ParameterValues = std::map<juce::String, float>;

struct Preset
{
    juce::String id;
    juce::String name;
    ParameterValues parameters;

    juce::var toJson() const;
    static juce::Result fromJson(const juce::var&, Preset&);
};

struct Bank
{
    static constexpr int currentFormatVersion = 1;

    juce::String id;
    juce::String name;
    bool factory = false;
    std::vector<Preset> presets;

    juce::var toJson() const;
    static juce::Result fromJson(const juce::var&, Bank&);
};
}

