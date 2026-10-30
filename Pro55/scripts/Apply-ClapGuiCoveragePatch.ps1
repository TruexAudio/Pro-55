param([Parameter(Mandatory=$true)][string]$ClapDir)
$ErrorActionPreference = 'Stop'
# Apply to a disposable copy of the pinned CLAP extension, for Pro-55 only.
$path = Join-Path $ClapDir 'src/wrapper/clap-juce-wrapper.cpp'
$text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
function Replace-Once([string]$before, [string]$after) {
    if ($script:text.Contains($after)) { return }
    if (($script:text.Split([string[]]@($before), [StringSplitOptions]::None)).Count -ne 2) {
        throw "Expected one CLAP patch location: $before"
    }
    $script:text = $script:text.Replace($before, $after)
}
Replace-Once '#include <juce_gui_basics/juce_gui_basics.h>' @'
#include <juce_gui_basics/juce_gui_basics.h>
#if JUCE_WINDOWS
#include "gui/ClapEditorCoverage.h"
#endif
'@
Replace-Once @'
                    setBounds(lastBounds);
                }
'@ @'
                    setBounds(lastBounds);
                }
                fitEditorToWrapper();
'@
$oldHelper = @'
        void fitEditorToWrapper()
        {
#if JUCE_WINDOWS
            if (editor != nullptr)
            {
                const juce::ScopedValueSetter<bool> resizingChildSetter{resizingChild, true};
                editor->setBounds(pro54::gui::editorBoundsForClapAllocation(*this, *editor));
            }
#endif
        }
'@
$newHelper = @'
        void fitEditorToWrapper()
        {
#if JUCE_WINDOWS
            if (editor != nullptr)
            {
                const juce::ScopedValueSetter<bool> resizingChildSetter{resizingChild, true};
                editor->setBounds(pro55::gui::editorBoundsForClapAllocation(*this, *editor));
            }
#endif
        }
'@
if ($script:text.Contains($oldHelper)) {
    $script:text = $script:text.Replace($oldHelper, $newHelper)
}
$duplicateHelper = $newHelper + "`n`n" + $newHelper
while ($script:text.Contains($duplicateHelper)) {
    $script:text = $script:text.Replace($duplicateHelper, $newHelper)
}
if (-not $script:text.Contains($newHelper)) {
    Replace-Once @'
        void resizeHostWindow()
        {
'@ ($newHelper + @'

        void resizeHostWindow()
        {
'@)
}
Replace-Once '                setBounds(editorBounds.withPosition(0, 0));' @'
                setBounds(editorBounds.withPosition(0, 0));
                // resized() may be skipped when the rounded allocation is unchanged,
                // or suppressed during the host's synchronous resize acknowledgement.
                fitEditorToWrapper();
'@
Replace-Once @'
                lastBounds = newBounds;
            }
        }

        HostType &host;
'@ @'
                lastBounds = newBounds;
            }
            // A different editor size can round to the same wrapper size.
            fitEditorToWrapper();
        }

        HostType &host;
'@
[IO.File]::WriteAllText($path, $text, [Text.UTF8Encoding]::new($false))
Write-Output 'Applied Windows CLAP editor coverage candidate patch.'
