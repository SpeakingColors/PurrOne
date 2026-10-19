#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace purrone;

PurrOneProcessor::PurrOneProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PurrOne", createLayout())
{
}

bool PurrOneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PurrOneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    eq.prepare (sampleRate);
    reverb.prepare (sampleRate);
    opto.prepare (sampleRate);
    maximizer.prepare (sampleRate);
    limiter.prepare (sampleRate, -1.0f);
    mono.assign ((size_t) std::max (samplesPerBlock, 1) + 64, 0.0f);
    verbMixSm.reset (sampleRate, 0.03);
    verbMixSm.setCurrentAndTargetValue (*apvts.getRawParameterValue (ids::verbMix));
    setLatencySamples (limiter.latency());
    applyParams();
}

static float raw (juce::AudioProcessorValueTreeState& a, const char* id) { return a.getRawParameterValue (id)->load(); }

void PurrOneProcessor::applyParams()
{
    Params p;
    auto& a = apvts;
    p.aOct = (int) raw (a, ids::aOct); p.aTune = raw (a, ids::aTune); p.aSaw = raw (a, ids::aSaw) > 0.5f; p.aPulse = raw (a, ids::aPulse) > 0.5f; p.aPW = raw (a, ids::aPW); p.aSync = raw (a, ids::aSync) > 0.5f;
    p.bOct = (int) raw (a, ids::bOct); p.bTune = raw (a, ids::bTune); p.bSaw = raw (a, ids::bSaw) > 0.5f; p.bTri = raw (a, ids::bTri) > 0.5f; p.bPulse = raw (a, ids::bPulse) > 0.5f; p.bPW = raw (a, ids::bPW);
    p.bLow = raw (a, ids::bLow) > 0.5f; p.bKbd = raw (a, ids::bKbd) > 0.5f;
    p.mixA = raw (a, ids::mixA); p.mixB = raw (a, ids::mixB); p.mixSub = raw (a, ids::mixSub); p.mixNoise = raw (a, ids::mixNoise);
    p.cutoffHz = raw (a, ids::cutoff); p.reso = raw (a, ids::reso); p.fenvAmt = raw (a, ids::fenvAmt); p.kbdAmt = raw (a, ids::kbd);
    p.fA = raw (a, ids::fA); p.fD = raw (a, ids::fD); p.fS = raw (a, ids::fS); p.fR = raw (a, ids::fR);
    p.aA = raw (a, ids::aA); p.aD = raw (a, ids::aD); p.aS = raw (a, ids::aS); p.aR = raw (a, ids::aR);
    p.glide = raw (a, ids::glide); p.glideAuto = raw (a, ids::glideAuto) > 0.5f;
    p.lfoHz = raw (a, ids::lfoHz); p.lfoSaw = raw (a, ids::lfoSaw) > 0.5f; p.lfoTri = raw (a, ids::lfoTri) > 0.5f; p.lfoPulse = raw (a, ids::lfoPulse) > 0.5f;
    p.modAmt[0] = raw (a, ids::mEnv); p.modRoute[0] = (int) raw (a, ids::mEnvR);
    p.modAmt[1] = raw (a, ids::mOscB); p.modRoute[1] = (int) raw (a, ids::mOscBR);
    p.modAmt[2] = raw (a, ids::mLfo); p.modRoute[2] = (int) raw (a, ids::mLfoR);
    p.modDest[0] = (int) raw (a, ids::dest0); p.modDest[1] = (int) raw (a, ids::dest1); p.modDest[2] = (int) raw (a, ids::dest2); p.modDest[3] = (int) raw (a, ids::dest3); p.modDest[4] = (int) raw (a, ids::dest4);
    p.modWheel = std::max (raw (a, ids::wheel), midiWheel);
    p.retrig = raw (a, ids::retrig) > 0.5f; p.repeat = raw (a, ids::repeat) > 0.5f; p.drone = raw (a, ids::drone) > 0.5f;
    p.drive = raw (a, ids::drive); p.xstr = raw (a, ids::xstr); p.fuzz = raw (a, ids::fuzz); p.volume = raw (a, ids::vol);
    engine.setParams (p);
    engine.setPitchBend (bendSemis);
    eq.setGains (raw (a, ids::eqLo), raw (a, ids::eqMid), raw (a, ids::eqHi));
    reverb.setParams (raw (a, ids::verbSize), raw (a, ids::verbHP));
    verbMixSm.setTargetValue (raw (a, ids::verbMix));
    opto.setAmount (raw (a, ids::opto));
    maximizer.setAmount (raw (a, ids::maxAmt));
    limiter.setEnabled (raw (a, ids::safety) > 0.5f);
}

void PurrOneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if ((int) mono.size() < n) return;                       // prepareToPlay not called with a big enough block; stay silent instead of allocating
    buffer.clear();

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos) { applyParams(); engine.process (mono.data() + pos, end - pos); pos = end; }
    };
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int at = std::min (meta.samplePosition, n);
        renderTo (at);
        if (m.isNoteOn()) engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
        else if (m.isPitchWheel()) bendSemis = (float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
        else if (m.isController() && m.getControllerNumber() == 1) midiWheel = (float) m.getControllerValue() / 127.0f;
    }
    renderTo (n);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    float pl = 0.0f, pr = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float x = eq.process (mono[(size_t) i]);
        float wl, wr; reverb.process (x, wl, wr);
        float mixv = verbMixSm.getNextValue();
        float l = x * (1.0f - mixv * 0.3f) + wl * mixv * 1.4f, r = x * (1.0f - mixv * 0.3f) + wr * mixv * 1.4f;
        opto.process (l, r);
        maximizer.process (l, r);
        limiter.process (l, r);
        if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0.0f; engine.reset(); }
        L[i] = l; R[i] = r;
        pl = std::max (pl, std::fabs (l)); pr = std::max (pr, std::fabs (r));
    }
    const float decay = std::pow (0.5f, (float) n / (float) (sr * 0.25));
    meterL.store (std::max (pl, meterL.load() * decay)); meterR.store (std::max (pr, meterR.load() * decay));
    if (std::max (pl, pr) >= 0.99f) clipped.store (true);
}

// ---- programs and presets ----
int PurrOneProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String PurrOneProcessor::getProgramName (int i) { auto& f = factoryPresets(); return i >= 0 && i < (int) f.size() ? juce::String (f[(size_t) i].name) : juce::String(); }
void PurrOneProcessor::setCurrentProgram (int i) { applyFactory (i); }

void PurrOneProcessor::applyFactory (int index)
{
    auto& list = factoryPresets();
    if (index < 0 || index >= (int) list.size()) return;
    // start from defaults so presets never inherit leftovers from the previous sound
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) r->setValueNotifyingHost (r->getDefaultValue());
    for (auto& kv : list[(size_t) index].values)
        if (auto* r = apvts.getParameter (kv.first)) r->setValueNotifyingHost (r->convertTo0to1 (kv.second));
    currentFactory = index; currentPresetName = list[(size_t) index].name;
}

juce::String PurrOneProcessor::sanitize (const juce::String& n)
{
    return juce::File::createLegalFileName (n.trim()).substring (0, 60);
}
juce::File PurrOneProcessor::userPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Studio 87").getChildFile ("PurrOne").getChildFile ("presets");
}
juce::StringArray PurrOneProcessor::listUserPresets() const
{
    juce::StringArray out;
    for (auto& f : userPresetDir().findChildFiles (juce::File::findFiles, false, "*.purrone")) out.add (f.getFileNameWithoutExtension());
    out.sort (true); return out;
}
bool PurrOneProcessor::saveUserPreset (const juce::String& name)
{
    auto clean = sanitize (name); if (clean.isEmpty()) return false;
    auto dir = userPresetDir(); if (! dir.createDirectory().wasOk()) return false;
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("presetName", clean); xml->setAttribute ("format", 1);
    currentPresetName = clean;
    return xml->writeTo (dir.getChildFile (clean + ".purrone"));
}
bool PurrOneProcessor::renameUserPreset (const juce::String& from, const juce::String& to)
{
    auto cleanTo = sanitize (to); if (cleanTo.isEmpty()) return false;
    auto src = userPresetDir().getChildFile (sanitize (from) + ".purrone");
    auto dst = userPresetDir().getChildFile (cleanTo + ".purrone");
    if (! src.existsAsFile() || (dst.existsAsFile() && dst != src)) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (src));
    if (xml == nullptr) return false;
    xml->setAttribute ("presetName", cleanTo);
    if (! xml->writeTo (dst)) return false;
    if (dst != src) src.deleteFile();
    if (currentPresetName == from) currentPresetName = cleanTo;
    return true;
}
bool PurrOneProcessor::loadUserPreset (const juce::String& name)
{
    auto file = userPresetDir().getChildFile (sanitize (name) + ".purrone");
    if (! file.existsAsFile() || file.getSize() > 1024 * 256) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    // reset to defaults, then apply only known parameters, clamped by the parameter ranges
    for (auto* prm : getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) r->setValueNotifyingHost (r->getDefaultValue());
    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
        if (auto* r = apvts.getParameter (child->getStringAttribute ("id")))
            r->setValueNotifyingHost (r->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    currentPresetName = name; return true;
}

// ---- state ----
void PurrOneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("format", 1); xml->setAttribute ("presetName", currentPresetName);
    copyXmlToBinary (*xml, dest);
}
void PurrOneProcessor::setStateInformation (const void* data, int size)
{
    if (size <= 0 || size > 1024 * 1024) return;
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    currentPresetName = xml->getStringAttribute ("presetName", "Init").substring (0, 60);
    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))              // only known ids, values clamped by the parameter
        if (auto* r = apvts.getParameter (child->getStringAttribute ("id")))
            r->setValueNotifyingHost (r->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
}

juce::AudioProcessorEditor* PurrOneProcessor::createEditor() { return new PurrOneEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PurrOneProcessor(); }
