#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "purrone/Engine.h"
#include "purrone/Fx.h"
#include "purrone/Fx2.h"
#include "purrone/Seq.h"
#include "Parameters.h"
#include <unordered_map>

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
    bool producesMidi() const override { return true; }     // only sends notes when MIDI out is switched on
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

    // ---- step sequencer data (4 patterns of up to 64 steps; saved in the host state and, if you want, inside sounds) ----
    purrone::SeqData seq;
    juce::String patternToString (int pat) const;                       // 4 hex digits per step (older saves have 3)
    void patternFromString (int pat, const juce::String& s);
    void applySeqSpec (const juce::String& spec);                        // factory notation, see Presets.h; fills pattern A and sets the step count
    static juce::File sequenceDir();
    juce::StringArray listSequences() const;
    bool saveSequence (const juce::String& name);                        // current pattern, rate, swing, gate, accent, slide and squelch
    bool loadSequence (const juce::String& name);
    std::atomic<int> accentCount { 0 };                                  // counts accented steps played, for the editor accent light
    std::atomic<int> playStep { -1 };                                    // for the editor playhead
    std::atomic<bool> seqRunning { false };
    std::atomic<float> uiBend { 0.0f }, bendShown { 0.0f };               // on-screen pitch wheel -> audio, and the wheel position (MIDI or screen) -> editor
    std::atomic<bool> uiBendNew { false };
    std::atomic<bool> playButton { false };                              // Play button on the Seq page (starts the sequence by hand)
    // analyser tap: the last 2048 output samples, written by the audio thread, read by the editor
    static constexpr int kTap = 4096;
    float tapL[kTap] {}, tapR[kTap] {}; std::atomic<int> tapPos { 0 };
    std::atomic<float> grOtt { 0.0f };
    int saveBank = 4;                                                    // bank (0 = A .. 7 = H) the next Save goes to; A to D are factory
    static juce::File userBankDir (int bank);
    juce::StringArray listBank (int bank) const;
    bool exportBank (int bank, const juce::File& f) const;
    int importBank (const juce::File& f, int bank);
    bool exportSound (const juce::File& f);
    bool importSound (const juce::File& f, int bank);
    juce::String sequenceCode (const juce::String& name);          // PO-SEQ: text to share
    bool importSequenceCode (const juce::String& code, const juce::String& name);
    bool deleteSequence (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    // ---- single fx module presets (the dropdown on each module) ----
    void applyValues (const std::vector<std::pair<const char*, float>>& v);
    // ---- favourites (a text file next to your sounds) and the random sound button ----
    juce::StringArray recentSounds;   // the sounds you loaded this session, newest first (shown in the preset browser; not saved)
    bool isFavourite (const juce::String& name) const { return favourites().contains (name); }
    void toggleFavourite (const juce::String& name);
    juce::StringArray favourites() const;
    void randomSound();                                                  // a bank A sound, then a gentle nudge on a few controls
    void setParam (const char* id, float realValue);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyState;                      // on-screen keyboard, mixed into the MIDI input
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };
    std::atomic<bool> clipped { false };
    juce::String currentPresetName { "Init" };
    int editorWidth = 1360;                                // window width, remembered while the host session runs

    const purrone::Eq5& eqView() const { return eq; }

    // ---- MIDI learn: right-click a knob or switch, then move a control on your controller. Saved in the host state. ----
    void startLearn (const juce::String& paramId);
    void cancelLearn() { learnTarget.store (-1); }
    juce::String learningId() const { const int i = learnTarget.load(); return i >= 0 && i < (int) learnParams.size() ? learnParams[(size_t) i]->paramID : juce::String(); }
    int ccFor (const juce::String& paramId) const;                       // -1 when not mapped
    void forgetCC (const juce::String& paramId);
    std::atomic<int> learnTarget { -1 };

private:
    void applyParams();
    mutable juce::StringArray favCache; mutable bool favLoaded = false;
    juce::File favouritesFile() const;
    void applyFactory (int index);
    static juce::String sanitize (const juce::String& n);

    std::vector<juce::RangedAudioParameter*> learnParams;
    std::atomic<int> ccMap[128];
    purrone::Engine engine;
    purrone::Eq5 eq;
    purrone::Crush crush;
    purrone::Ott ott;
    purrone::DanceMaster dance;
    purrone::StereoTool stereo;
    purrone::Sequencer sequencer;
    std::unordered_map<const char*, std::atomic<float>*> rawCache;
    float rd (const char* id);
    void applyFx();
    void applySeqSettings();
    void applyTuning (purrone::Params& p);
    bool passed[128] {};
    bool eqPre = false, eqPost = false, spaceOn = true; float duckAmt = 0.0f, duckEnv = 0.0f;
    int lastOversample = 4, lastBend = 0; int lastOut = -1; bool wheelIsCC74 = false;
    purrone::Reverb reverb;
    purrone::Opto opto;
    purrone::Maximizer maximizer;
    purrone::SafetyLimiter limiter;
    std::vector<float> mono;
    juce::SmoothedValue<float> verbMixSm;
    float midiAT = 0.0f, midiWheel = 0.0f, bendNorm = 0.0f, bendRange = 2.0f, slideSeconds = 0.08f;
    bool midiOutOn = false; int midiOutCh = 1, midiOutVel = 90, midiOutAcc = 120;
    juce::MidiBuffer outMidi;
    void resetFx();
    int currentFactory = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurrOneProcessor)
};
