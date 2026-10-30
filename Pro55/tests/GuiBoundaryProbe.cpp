#include <juce_gui_basics/juce_gui_basics.h>
#include <windows.h>
#include <iostream>
#include "../src/gui/ClapEditorCoverage.h"

static int checkClapCoverage()
{
    int originalGaps = 0;
    int correctedGaps = 0;
    int cases = 0;
    for (float scale : {1.0f, 1.25f, 1.5f, 1.75f, 2.0f})
    {
        for (int width = 1101; width <= 1111; ++width)
        {
            juce::Component wrapper, editor;
            wrapper.addChildComponent(editor);
            editor.setTransform(juce::AffineTransform::scale(scale));
            editor.setBounds(0, 0, width, width / 2);
            // An editor-originated resize rounds its transformed size into a
            // wrapper allocation. Creation and unchanged-allocation paths can
            // bypass resized(), leaving the old editor size inside that allocation.
            wrapper.setBounds(wrapper.getLocalArea(&editor, editor.getLocalBounds()).withZeroOrigin());
            const auto covers = [&]
            {
                const auto area = wrapper.getLocalArea(&editor, editor.getLocalBounds().toFloat());
                return area.getRight() + 0.0001f >= static_cast<float>(wrapper.getWidth())
                    && area.getBottom() + 0.0001f >= static_cast<float>(wrapper.getHeight());
            };
            if (!covers()) ++originalGaps;
            editor.setBounds(pro55::gui::editorBoundsForClapAllocation(wrapper, editor));
            if (!covers()) ++correctedGaps;
            ++cases;
        }
    }
    std::cout << "CLAP transformed-allocation cases=" << cases
              << ", original gaps=" << originalGaps
              << ", corrected gaps=" << correctedGaps << '\n';
    return originalGaps > 0 && correctedGaps == 0 ? 0 : 4;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 1 && std::string(argv[1]) == "--clap")
        return checkClapCoverage();
    auto parent = CreateWindowExW(0, L"STATIC", L"Boundary probe", WS_POPUP,
                                 0, 0, 2200, 1200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (parent == nullptr) return 2;
    {
        juce::Component child;
        child.setOpaque(true);
        child.setBounds(0, 0, 1000, 500);
        child.addToDesktop(0, parent);
        auto* peer = child.getPeer();
        std::cout << "scale,requestedPhysicalWidth,nativeWidth,logicalWidth,paintCoverageWidth,nativeHeight,paintCoverageHeight\n";
        for (double scale : {1.0, 1.25, 1.5, 1.75, 2.0})
        {
            peer->setCustomPlatformScaleFactor(scale);
            for (int width = 1808; width <= 1818; ++width)
            {
                // With no argument, reproduce the original VST3 onSize route.
                // With --outward, verify the candidate Windows allocation route.
                const auto logicalRequest = juce::Rectangle<int>{width, 850}.toFloat() / scale;
                peer->setBounds(argc > 1 ? logicalRequest.getSmallestIntegerContainer() : logicalRequest.toNearestInt(), false);
                RECT native{};
                GetClientRect(static_cast<HWND>(peer->getNativeHandle()), &native);
                const auto logical = peer->getBounds();
                std::cout << scale << ',' << width << ',' << native.right << ','
                          << logical.getWidth() << ',' << logical.getWidth() * scale << ','
                          << native.bottom << ',' << logical.getHeight() * scale << '\n';
                if (argc > 1 && (native.right < width || native.bottom < 850
                                || logical.getWidth() * scale < native.right
                                || logical.getHeight() * scale < native.bottom))
                    return 3;
            }
        }
        child.removeFromDesktop();
    }
    DestroyWindow(parent);
}
