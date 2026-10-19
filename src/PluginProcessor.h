#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "purrone/Engine.h"
#include "purrone/Fx.h"
#include "Parameters.h"

class PurrOneProcessor : public juce::AudioProcessor
{
public:
    PurrOneProcessor();
    ~PurrOneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "PurrOne"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.5; }   // longest reverb setting plus limiter

    // factory presets are exposed as host programs
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentFactory; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // user presets: one XML file per sound in the user's application data folder
    static juce::File userPresetDir();
    juce::StringArray listUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool renameUserPreset (const juce::String& from, const juce::String& to);   // user presets only
    bool loadUserPreset (const juce::String& name);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyState;                      // on-screen keyboard, mixed into the MIDI input
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };
    std::atomic<bool> clipped { false };
    juce::String currentPresetName { "Init" };
    int editorWidth = 1360;                                // window width, remembered while the host session runs

private:
    void applyParams();
    void applyFactory (int index);
    static juce::String sanitize (const juce::String& n);

    purrone::Engine engine;
    purrone::Eq3 eq;
    purrone::Reverb reverb;
    purrone::Opto opto;
    purrone::Maximizer maximizer;
    purrone::SafetyLimiter limiter;
    std::vector<float> mono;
    juce::SmoothedValue<float> verbMixSm;
    float midiWheel = 0.0f, bendSemis = 0.0f;
    int currentFactory = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurrOneProcessor)
};
