// Parameter table shared by the processor, the presets and the tests. IDs are stable forever.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "purrone/Engine.h"

namespace purrone::ids
{
    // oscillators
    inline constexpr const char* aOct = "aOct"; inline constexpr const char* aTune = "aTune"; inline constexpr const char* aSaw = "aSaw";
    inline constexpr const char* aPulse = "aPulse"; inline constexpr const char* aPW = "aPW"; inline constexpr const char* aSync = "aSync";
    inline constexpr const char* bOct = "bOct"; inline constexpr const char* bTune = "bTune"; inline constexpr const char* bSaw = "bSaw";
    inline constexpr const char* bTri = "bTri"; inline constexpr const char* bPulse = "bPulse"; inline constexpr const char* bPW = "bPW";
    inline constexpr const char* bLow = "bLow"; inline constexpr const char* bKbd = "bKbd";
    inline constexpr const char* mixA = "mixA"; inline constexpr const char* mixB = "mixB"; inline constexpr const char* mixSub = "mixSub"; inline constexpr const char* mixNoise = "mixNoise";
    // filter and envelopes
    inline constexpr const char* cutoff = "cutoff"; inline constexpr const char* reso = "reso"; inline constexpr const char* fenvAmt = "fenvAmt"; inline constexpr const char* kbd = "kbd";
    inline constexpr const char* fA = "fA"; inline constexpr const char* fD = "fD"; inline constexpr const char* fS = "fS"; inline constexpr const char* fR = "fR";
    inline constexpr const char* aA = "aA"; inline constexpr const char* aD = "aD"; inline constexpr const char* aS = "aS"; inline constexpr const char* aR = "aR";
    // lfo, matrix, keyboard
    inline constexpr const char* lfoHz = "lfoHz"; inline constexpr const char* lfoSaw = "lfoSaw"; inline constexpr const char* lfoTri = "lfoTri"; inline constexpr const char* lfoPulse = "lfoPulse";
    inline constexpr const char* mEnv = "mEnv"; inline constexpr const char* mEnvR = "mEnvR"; inline constexpr const char* mOscB = "mOscB"; inline constexpr const char* mOscBR = "mOscBR";
    inline constexpr const char* mLfo = "mLfo"; inline constexpr const char* mLfoR = "mLfoR";
    inline constexpr const char* dest0 = "dest0"; inline constexpr const char* dest1 = "dest1"; inline constexpr const char* dest2 = "dest2"; inline constexpr const char* dest3 = "dest3"; inline constexpr const char* dest4 = "dest4";
    inline constexpr const char* wheel = "wheel"; inline constexpr const char* glide = "glide"; inline constexpr const char* glideAuto = "glideAuto";
    inline constexpr const char* retrig = "retrig"; inline constexpr const char* repeat = "repeat"; inline constexpr const char* drone = "drone";
    // fx and output
    inline constexpr const char* drive = "drive"; inline constexpr const char* xstr = "xstr"; inline constexpr const char* fuzz = "fuzz";
    inline constexpr const char* eqLo = "eqLo"; inline constexpr const char* eqMid = "eqMid"; inline constexpr const char* eqHi = "eqHi";
    inline constexpr const char* verbMix = "verbMix"; inline constexpr const char* verbSize = "verbSize"; inline constexpr const char* verbHP = "verbHP";
    inline constexpr const char* opto = "opto"; inline constexpr const char* maxAmt = "maxAmt"; inline constexpr const char* safety = "safety"; inline constexpr const char* vol = "vol";
}

namespace purrone
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    using F = AudioParameterFloat; using B = AudioParameterBool; using C = AudioParameterChoice;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pid = [] (const char* id) { return ParameterID { id, 1 }; };
    auto lin = [&] (const char* id, const char* name, float lo, float hi, float def) { p.push_back (std::make_unique<F> (pid (id), name, NormalisableRange<float> (lo, hi), def)); };
    auto b = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<B> (pid (id), name, def)); };
    auto oct = [&] (const char* id, const char* name) { p.push_back (std::make_unique<C> (pid (id), name, StringArray { "16'", "8'", "4'", "2'" }, 1)); };
    auto route = [&] (const char* id, const char* name, int def) { p.push_back (std::make_unique<C> (pid (id), name, StringArray { "Off", "Direct", "Wheel" }, def)); };

    oct (ids::aOct, "A Range"); lin (ids::aTune, "A Tune", -6.0f, 6.0f, 0.0f); b (ids::aSaw, "A Saw", true); b (ids::aPulse, "A Pulse", false); lin (ids::aPW, "A Width", 0.05f, 0.95f, 0.5f); b (ids::aSync, "A Sync", false);
    oct (ids::bOct, "B Range"); lin (ids::bTune, "B Tune", -6.0f, 6.0f, 0.0f); b (ids::bSaw, "B Saw", true); b (ids::bTri, "B Triangle", false); b (ids::bPulse, "B Pulse", false); lin (ids::bPW, "B Width", 0.05f, 0.95f, 0.5f);
    b (ids::bLow, "B Low Freq", false); b (ids::bKbd, "B Keys", true);
    lin (ids::mixA, "Mix A", 0, 1, 0.8f); lin (ids::mixB, "Mix B", 0, 1, 0.6f); lin (ids::mixSub, "Mix Sub", 0, 1, 0.0f); lin (ids::mixNoise, "Mix Noise", 0, 1, 0.0f);

    { NormalisableRange<float> r (20.0f, 20000.0f); r.setSkewForCentre (1000.0f); p.push_back (std::make_unique<F> (pid (ids::cutoff), "Cutoff", r, 2400.0f)); }
    lin (ids::reso, "Resonance", 0, 1, 0.15f); lin (ids::fenvAmt, "Filter Env Amount", -1, 1, 0.35f); lin (ids::kbd, "Key Track", 0, 1, 0.5f);
    lin (ids::fA, "F Attack", 0, 1, 0.0f); lin (ids::fD, "F Decay", 0, 1, 0.45f); lin (ids::fS, "F Sustain", 0, 1, 0.25f); lin (ids::fR, "F Release", 0, 1, 0.3f);
    lin (ids::aA, "A Attack", 0, 1, 0.0f); lin (ids::aD, "A Decay", 0, 1, 0.45f); lin (ids::aS, "A Sustain", 0, 1, 0.8f); lin (ids::aR, "A Release", 0, 1, 0.3f);

    { NormalisableRange<float> r (0.1f, 30.0f); r.setSkewForCentre (3.0f); p.push_back (std::make_unique<F> (pid (ids::lfoHz), "LFO Rate", r, 5.0f)); }
    b (ids::lfoSaw, "LFO Saw", false); b (ids::lfoTri, "LFO Triangle", true); b (ids::lfoPulse, "LFO Pulse", false);
    lin (ids::mEnv, "Mod Filter Env", 0, 1, 0.0f); route (ids::mEnvR, "Mod Filter Env Route", 0);
    lin (ids::mOscB, "Mod Osc B", 0, 1, 0.0f); route (ids::mOscBR, "Mod Osc B Route", 0);
    lin (ids::mLfo, "Mod LFO", 0, 1, 0.3f); route (ids::mLfoR, "Mod LFO Route", 2);
    route (ids::dest0, "To A Freq", 0); route (ids::dest1, "To A Width", 0); route (ids::dest2, "To B Freq", 0); route (ids::dest3, "To B Width", 0); route (ids::dest4, "To Filter", 0);
    lin (ids::wheel, "Mod Wheel", 0, 1, 0.0f); lin (ids::glide, "Glide", 0, 1, 0.0f); b (ids::glideAuto, "Auto Glide", false);
    b (ids::retrig, "Retrigger", false); b (ids::repeat, "Repeat", false); b (ids::drone, "Drone", false);

    lin (ids::drive, "Drive", 0, 1, 0.0f); lin (ids::xstr, "Transistor", 0, 1, 0.1f); lin (ids::fuzz, "Fuzz", 0, 1, 0.0f);
    lin (ids::eqLo, "EQ Low", -12, 12, 0); lin (ids::eqMid, "EQ Mid", -12, 12, 0); lin (ids::eqHi, "EQ High", -12, 12, 0);
    lin (ids::verbMix, "Space Mix", 0, 1, 0.12f); lin (ids::verbSize, "Space Length", 0.2f, 4.0f, 0.9f);
    { NormalisableRange<float> r (80.0f, 600.0f); r.setSkewForCentre (220.0f); p.push_back (std::make_unique<F> (pid (ids::verbHP), "Space Low Cut", r, 250.0f)); }
    lin (ids::opto, "Opto", 0, 1, 0.0f); lin (ids::maxAmt, "Maximizer", 0, 1, 0.0f); b (ids::safety, "Safety Limiter", true); lin (ids::vol, "Volume", 0, 1, 0.6f);
    return { p.begin(), p.end() };
}
} // namespace purrone
