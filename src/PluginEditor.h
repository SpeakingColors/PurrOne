#pragma once
#include "PluginProcessor.h"
#include "ui/Widgets.h"
#include "ui/Pages.h"

// The PurrOne window. Layout, colours and wording follow docs/mockup/purrone-mockup.html; it is drawn in a fixed
// 1700 x 828 space and scaled to the window size (aspect ratio locked).
class PurrOneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PurrOneEditor (PurrOneProcessor&);
    ~PurrOneEditor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (purrone::ui::col::ground); }
private:
    struct Look; struct Content; struct Button; struct NameBox; struct Check;
    void timerCallback() override;
    void refreshNames();
    void step (int dir);
    void openList();
    void beginRename();
    void commitRename();
    void saveDialog();
    void saveNamed (const juce::String& name);
    void cycleZoom();
    void setZoomPercent (int pct);
    int zoomPercent() const;
    void showView (int v);
    void resetZoom();
    static constexpr int kDefaultW = 1360;   // the 100% size

    PurrOneProcessor& proc;
    purrone::ui::Bubbles bubbles;
    std::unique_ptr<Look> look;
    std::unique_ptr<Content> content;
    std::unique_ptr<juce::TooltipWindow> tooltip;
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableBorderComponent> border;
    std::unique_ptr<juce::ResizableCornerComponent> corner;
    juce::StringArray names;           // factory first, then user presets
    int numFactory = 0;
    std::unique_ptr<juce::AlertWindow> dialog;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurrOneEditor)
};
