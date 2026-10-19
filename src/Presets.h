// Factory presets. Values use the parameter IDs in Parameters.h; anything not listed keeps its default.
// Generated from docs/mockup first, then tuned by ear. Original sounds, no copies of other makers' presets.
#pragma once
#include <vector>
#include <utility>

namespace purrone
{
struct FactoryPreset { const char* name; const char* category; std::vector<std::pair<const char*, float>> values; };

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list = {
        { "Init", "Init", {  } },
        { "Deep Sub", "Sub", { {"aSaw", 0.0f}, {"aPulse", 0.0f}, {"bSaw", 0.0f}, {"mixA", 0.0f}, {"mixB", 0.0f}, {"mixSub", 0.95f}, {"aOct", 0.0f}, {"cutoff", 500.0f}, {"reso", 0.05f}, {"fenvAmt", 0.1f}, {"aS", 1.0f}, {"aD", 0.5f}, {"xstr", 0.05f}, {"eqLo", 2.0f}, {"opto", 0.5f}, {"bOct", 1.0f}, {"bTune", 0.0f}, {"fS", 0.25f}, {"lfoHz", 5.0f}, {"mLfo", 0.3f}, {"mLfoR", 2.0f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f} } },
        { "Clean Sub Thump", "Sub", { {"aSaw", 0.0f}, {"aPulse", 1.0f}, {"aPW", 0.5f}, {"bSaw", 0.0f}, {"mixA", 0.55f}, {"mixB", 0.0f}, {"mixSub", 0.8f}, {"aOct", 0.0f}, {"cutoff", 380.0f}, {"reso", 0.1f}, {"fenvAmt", 0.5f}, {"fD", 0.28f}, {"fS", 0.0f}, {"aD", 0.4f}, {"aS", 0.7f}, {"xstr", 0.1f}, {"bOct", 1.0f}, {"bTune", 0.0f}, {"lfoHz", 5.0f}, {"mLfo", 0.3f}, {"mLfoR", 2.0f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Round Bass", "Bass", { {"aSaw", 1.0f}, {"bSaw", 0.0f}, {"bTri", 1.0f}, {"bOct", 0.0f}, {"mixA", 0.7f}, {"mixB", 0.5f}, {"mixSub", 0.35f}, {"cutoff", 700.0f}, {"reso", 0.25f}, {"fenvAmt", 0.55f}, {"fD", 0.35f}, {"fS", 0.1f}, {"aS", 0.85f}, {"xstr", 0.15f}, {"kbd", 0.4f}, {"aOct", 1.0f}, {"bTune", 0.0f}, {"lfoHz", 5.0f}, {"mLfo", 0.3f}, {"mLfoR", 2.0f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Dirty Reese", "Bass", { {"aOct", 0.0f}, {"bOct", 0.0f}, {"bTune", 0.4f}, {"mixA", 0.7f}, {"mixB", 0.7f}, {"mixSub", 0.4f}, {"cutoff", 585.0f}, {"reso", 0.3f}, {"fenvAmt", 0.2f}, {"fS", 0.5f}, {"aS", 1.0f}, {"lfoHz", 0.4f}, {"mLfo", 0.25f}, {"mLfoR", 1.0f}, {"xstr", 0.49f}, {"fuzz", 0.37f}, {"eqMid", 2.3f}, {"eqHi", -3.0f}, {"opto", 0.5f} } },
        { "Nasty 808 Dub", "Bass", { {"cutoff", 25.0f}, {"fD", 0.72f}, {"fS", 0.21f}, {"aD", 0.38f}, {"aS", 0.56f}, {"bTune", 0.4f} } },
        { "Pluck Bass", "Bass", { {"aSaw", 1.0f}, {"bSaw", 0.0f}, {"mixA", 0.8f}, {"mixB", 0.0f}, {"mixSub", 0.4f}, {"cutoff", 420.0f}, {"reso", 0.35f}, {"fenvAmt", 0.85f}, {"fD", 0.3f}, {"fS", 0.0f}, {"aD", 0.35f}, {"aS", 0.2f}, {"aR", 0.2f}, {"xstr", 0.2f}, {"aOct", 1.0f}, {"bOct", 1.0f}, {"bTune", 0.0f}, {"lfoHz", 5.0f}, {"mLfo", 0.3f}, {"mLfoR", 2.0f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Acid Squelch", "Bass", { {"aSaw", 1.0f}, {"mixA", 0.9f}, {"mixB", 0.0f}, {"cutoff", 350.0f}, {"reso", 0.82f}, {"fenvAmt", 0.75f}, {"fD", 0.33f}, {"fS", 0.0f}, {"aD", 0.4f}, {"aS", 0.7f}, {"glide", 0.25f}, {"glideAuto", 1.0f}, {"xstr", 0.25f}, {"kbd", 0.2f}, {"aOct", 1.0f}, {"bOct", 1.0f}, {"bTune", 0.0f}, {"mixSub", 0.0f}, {"lfoHz", 5.0f}, {"mLfo", 0.3f}, {"mLfoR", 2.0f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Sync Lead", "Lead", { {"aSaw", 1.0f}, {"aSync", 1.0f}, {"bSaw", 1.0f}, {"aTune", 5.0f}, {"bOct", 1.0f}, {"mixA", 0.8f}, {"mixB", 0.2f}, {"cutoff", 6500.0f}, {"reso", 0.15f}, {"fenvAmt", 0.25f}, {"aS", 0.9f}, {"glide", 0.15f}, {"mLfo", 0.1f}, {"mLfoR", 2.0f}, {"dest0", 2.0f}, {"verbMix", 0.2f}, {"verbSize", 1.6f}, {"aOct", 1.0f}, {"bTune", 0.0f}, {"mixSub", 0.0f}, {"fS", 0.25f}, {"lfoHz", 5.0f}, {"xstr", 0.1f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Soft Lead", "Lead", { {"aSaw", 1.0f}, {"bSaw", 0.0f}, {"bTri", 1.0f}, {"bTune", 0.1f}, {"mixA", 0.7f}, {"mixB", 0.5f}, {"cutoff", 3800.0f}, {"reso", 0.1f}, {"fenvAmt", 0.2f}, {"fA", 0.1f}, {"aA", 0.08f}, {"aS", 0.9f}, {"aR", 0.4f}, {"lfoHz", 5.5f}, {"mLfo", 0.05f}, {"mLfoR", 2.0f}, {"dest0", 2.0f}, {"wheel", 0.4f}, {"verbMix", 0.25f}, {"verbSize", 2.4f}, {"eqHi", -2.0f}, {"aOct", 1.0f}, {"bOct", 1.0f}, {"mixSub", 0.0f}, {"fS", 0.25f}, {"xstr", 0.1f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"opto", 0.0f} } },
        { "Hollow Pulse", "Lead", { {"aSaw", 0.0f}, {"aPulse", 1.0f}, {"aPW", 0.3f}, {"bSaw", 0.0f}, {"bPulse", 1.0f}, {"bPW", 0.6f}, {"bTune", 0.08f}, {"mixA", 0.6f}, {"mixB", 0.5f}, {"cutoff", 2600.0f}, {"reso", 0.2f}, {"fenvAmt", 0.3f}, {"fD", 0.5f}, {"fS", 0.3f}, {"lfoHz", 0.8f}, {"mLfo", 0.3f}, {"mLfoR", 1.0f}, {"dest1", 1.0f}, {"dest3", 1.0f}, {"verbMix", 0.15f}, {"aOct", 1.0f}, {"bOct", 1.0f}, {"mixSub", 0.0f}, {"aS", 0.8f}, {"xstr", 0.1f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
        { "Drone Texture", "FX", { {"aSaw", 1.0f}, {"bSaw", 1.0f}, {"bTune", 7.0f}, {"mixA", 0.6f}, {"mixB", 0.6f}, {"mixNoise", 0.1f}, {"cutoff", 1400.0f}, {"reso", 0.5f}, {"fenvAmt", 0.2f}, {"drone", 1.0f}, {"lfoHz", 0.3f}, {"mLfo", 0.5f}, {"mLfoR", 1.0f}, {"dest4", 1.0f}, {"verbMix", 0.4f}, {"verbSize", 3.5f}, {"aA", 0.5f}, {"aR", 0.7f}, {"aOct", 1.0f}, {"bOct", 1.0f}, {"mixSub", 0.0f}, {"fS", 0.25f}, {"aS", 0.8f}, {"xstr", 0.1f}, {"fuzz", 0.0f}, {"eqMid", 0.0f}, {"eqHi", 0.0f}, {"opto", 0.0f} } },
    };
    return list;
}
} // namespace purrone
