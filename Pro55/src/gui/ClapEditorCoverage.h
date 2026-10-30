#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pro55::gui
{
// Work in floating point until the final outward rounding. The CLAP window
// allocation is already an integer, but its editor can have a fractional scale.
inline juce::Rectangle<int> editorBoundsForClapAllocation(const juce::Component& wrapper,
                                                         const juce::Component& editor)
{
    return editor.getLocalArea(&wrapper, wrapper.getLocalBounds().toFloat())
                 .getSmallestIntegerContainer().withZeroOrigin();
}
}
