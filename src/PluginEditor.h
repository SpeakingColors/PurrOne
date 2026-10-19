#pragma once
#include "PluginProcessor.h"

// Working editor: preset manager, parameter list, output meter, wood cheeks. The final interface follows docs/mockup.
class PurrOneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PurrOneEditor (PurrOneProcessor&);
    ~PurrOneEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void refreshPresetBox();
    void step (int dir);

    PurrOneProcessor& proc;
    void mouseDoubleClick (const juce::MouseEvent& e) override { if (e.eventComponent == &presetBox || presetBox.isParentOf (e.eventComponent)) renameBtn.onClick(); }
    juce::ComboBox presetBox; juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" }, renameBtn { "Rename" };
    juce::GenericAudioProcessorEditor generic;
    float dispL = 0, dispR = 0, holdL = 0, holdR = 0; int clipFrames = 0;
    juce::Rectangle<int> meterArea;
    juce::StringArray userNames;
    std::unique_ptr<juce::AlertWindow> saveDialog;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurrOneEditor)
};
