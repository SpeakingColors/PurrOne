#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace purrone;

PurrOneProcessor::PurrOneProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PurrOne", createLayout())
{
    rawCache.reserve (512);
    for (auto& c : ccMap) c.store (-1);
    for (auto* q : getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (q)) learnParams.push_back (r);
    outMidi.ensureSize (4096);
}

bool PurrOneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// Parameter lookup by pointer key. The first pass (in prepareToPlay) fills the table, so the audio thread never allocates.
float PurrOneProcessor::rd (const char* id)
{
    auto it = rawCache.find (id);
    if (it == rawCache.end()) it = rawCache.emplace (id, apvts.getRawParameterValue (id)).first;
    return it->second->load();
}

void PurrOneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.setOversampling (rd (ids::oversmp) > 0.5f ? 8 : 4);
    engine.prepare (sampleRate);
    eq.prepare (sampleRate);
    crush.prepare (sampleRate);
    reverb.prepare (sampleRate);
    opto.prepare (sampleRate);
    ott.prepare (sampleRate);
    dance.prepare (sampleRate);
    maximizer.prepare (sampleRate);
    stereo.prepare (sampleRate);
    limiter.prepare (sampleRate, -1.0f);
    sequencer.prepare (sampleRate);
    std::fill (std::begin (passed), std::end (passed), false);
    duckEnv = 0.0f; lastOut = -1;
    mono.assign ((size_t) std::max (samplesPerBlock, 1) + 64, 0.0f);
    verbMixSm.reset (sampleRate, 0.03);
    verbMixSm.setCurrentAndTargetValue (rd (ids::verbMix));
    setLatencySamples (limiter.latency());
    applyParams(); applyFx(); applySeqSettings();      // also fills the parameter table
}

void PurrOneProcessor::applyTuning (Params& p)
{
    static const double just[12] = { 1, 16.0 / 15, 9.0 / 8, 6.0 / 5, 5.0 / 4, 4.0 / 3, 45.0 / 32, 3.0 / 2, 8.0 / 5, 5.0 / 3, 16.0 / 9, 15.0 / 8 };
    static const double pyth[12] = { 1, 256.0 / 243, 9.0 / 8, 32.0 / 27, 81.0 / 64, 4.0 / 3, 729.0 / 512, 3.0 / 2, 128.0 / 81, 27.0 / 16, 16.0 / 9, 243.0 / 128 };
    static const char* custom[12] = { ids::tuneC0, ids::tuneC1, ids::tuneC2, ids::tuneC3, ids::tuneC4, ids::tuneC5, ids::tuneC6, ids::tuneC7, ids::tuneC8, ids::tuneC9, ids::tuneC10, ids::tuneC11 };
    const int t = (int) std::lround (rd (ids::tuneTemp));
    for (int i = 0; i < 12; ++i)
    {
        double cents = 0.0;
        if (t == 1) cents = 1200.0 * std::log2 (just[i]) - 100.0 * i;
        else if (t == 2) cents = 1200.0 * std::log2 (pyth[i]) - 100.0 * i;
        else if (t == 3) cents = (double) rd (custom[i]);
        p.tuneOff[i] = (float) (cents / 100.0);
    }
    static const double ref[3] = { 432.0, 440.0, 442.0 };
    p.tuneRef = (float) (12.0 * std::log2 (ref[std::min (std::max ((int) std::lround (rd (ids::tuneRef)), 0), 2)] / 440.0));
}

void PurrOneProcessor::applyParams()
{
    Params p;
    auto raw = [this] (const char* id) { return rd (id); };
    p.aOct = (int) raw (ids::aOct); p.aTune = raw (ids::aTune); p.aSaw = raw (ids::aSaw) > 0.5f; p.aPulse = raw (ids::aPulse) > 0.5f; p.aPW = raw (ids::aPW); p.aSync = raw (ids::aSync) > 0.5f;
    p.bOct = (int) raw (ids::bOct); p.bTune = raw (ids::bTune); p.bSaw = raw (ids::bSaw) > 0.5f; p.bTri = raw (ids::bTri) > 0.5f; p.bPulse = raw (ids::bPulse) > 0.5f; p.bPW = raw (ids::bPW);
    p.bLow = raw (ids::bLow) > 0.5f; p.bKbd = raw (ids::bKbd) > 0.5f;
    p.mixA = raw (ids::mixA); p.mixB = raw (ids::mixB); p.mixSub = raw (ids::mixSub); p.mixNoise = raw (ids::mixNoise);
    p.filterMode = (int) std::lround (raw (ids::fMode)); p.colourPos = (int) std::lround (raw (ids::colPos)); p.cutoffHz = raw (ids::cutoff); p.reso = raw (ids::reso); p.fenvAmt = raw (ids::fenvAmt); p.kbdAmt = raw (ids::kbd);
    p.fA = raw (ids::fA); p.fD = raw (ids::fD); p.fS = raw (ids::fS); p.fR = raw (ids::fR);
    p.aA = raw (ids::aA); p.aD = raw (ids::aD); p.aS = raw (ids::aS); p.aR = raw (ids::aR);
    p.glide = raw (ids::glide); p.glideAuto = raw (ids::glideAuto) > 0.5f;
    p.lfoHz = raw (ids::lfoHz); p.lfoSaw = raw (ids::lfoSaw) > 0.5f; p.lfoTri = raw (ids::lfoTri) > 0.5f; p.lfoPulse = raw (ids::lfoPulse) > 0.5f;
    p.modAmt[0] = raw (ids::mEnv); p.modRoute[0] = (int) raw (ids::mEnvR);
    p.modAmt[1] = raw (ids::mOscB); p.modRoute[1] = (int) raw (ids::mOscBR);
    p.modAmt[2] = raw (ids::mLfo); p.modRoute[2] = (int) raw (ids::mLfoR);
    p.modDest[0] = (int) raw (ids::dest0); p.modDest[1] = (int) raw (ids::dest1); p.modDest[2] = (int) raw (ids::dest2); p.modDest[3] = (int) raw (ids::dest3); p.modDest[4] = (int) raw (ids::dest4);
    p.modWheel = std::max (raw (ids::wheel), midiWheel);
    p.retrig = raw (ids::retrig) > 0.5f; p.repeat = raw (ids::repeat) > 0.5f; p.drone = raw (ids::drone) > 0.5f;
    p.drive = raw (ids::drive); p.xstr = raw (ids::xstr); p.fuzz = raw (ids::fuzz); p.volume = raw (ids::vol);
    p.squelch = raw (ids::squelch) > 0.5f; p.squelchAmt = raw (ids::squelchAmt); p.accAmt = raw (ids::seqAcc);
    p.rndHz = raw (ids::rndHz); p.rndSlew = raw (ids::rndSlew); p.rndDepth = raw (ids::rndDepth);
    p.rndCut = raw (ids::rndCut) > 0.5f; p.rndPA = raw (ids::rndPA) > 0.5f; p.rndPB = raw (ids::rndPB) > 0.5f; p.rndPW = raw (ids::rndPW) > 0.5f;
    applyTuning (p);
    const int os = raw (ids::oversmp) > 0.5f ? 8 : 4;
    if (os != lastOversample) { lastOversample = os; engine.setOversampling (os); }
    engine.setParams (p);
    static const float bends[4] = { 2.0f, 5.0f, 12.0f, 24.0f };   // semitones up and down at full wheel; 12 is an octave, 24 two octaves
    bendRange = bends[std::min (std::max ((int) std::lround (raw (ids::bend)), 0), 3)];
    wheelIsCC74 = raw (ids::wheelCC) > 0.5f;
    engine.setPitchBend (bendNorm * bendRange);
}

void PurrOneProcessor::applyFx()
{
    auto raw = [this] (const char* id) { return rd (id); };
    crush.set (raw (ids::crushOn) > 0.5f, raw (ids::crushBits), raw (ids::crushRate), raw (ids::crushMix));
    struct B { const char* g; const char* f; const char* q; const char* d; const char* t; const char* m; const char* o; };
    static const B bands[5] = {
        { ids::eqLo, ids::eqLoF, nullptr, ids::eqD1, ids::eqT1, ids::eqM1, ids::eqO1 },
        { ids::eqG2, ids::eqF2, ids::eqQ2, ids::eqD2, ids::eqT2, ids::eqM2, ids::eqO2 },
        { ids::eqMid, ids::eqMidF, ids::eqMidQ, ids::eqD3, ids::eqT3, ids::eqM3, ids::eqO3 },
        { ids::eqG4, ids::eqF4, ids::eqQ4, ids::eqD4, ids::eqT4, ids::eqM4, ids::eqO4 },
        { ids::eqHi, ids::eqHiF, nullptr, ids::eqD5, ids::eqT5, ids::eqM5, ids::eqO5 } };
    for (int i = 0; i < 5; ++i)
        eq.setBand (i, raw (bands[i].o) > 0.5f, raw (bands[i].f), raw (bands[i].g), bands[i].q ? raw (bands[i].q) : 0.7071f, raw (bands[i].d), raw (bands[i].t), (int) std::lround (raw (bands[i].m)));
    eqPre = eq.anyMono(); eqPost = eq.anyStereo();
    reverb.setParams (raw (ids::verbSize), raw (ids::verbHP));
    reverb.setExtras (raw (ids::spPre), raw (ids::spDamp), raw (ids::spWid));
    spaceOn = raw (ids::fxOnSpace) > 0.5f; duckAmt = raw (ids::spDuck);
    verbMixSm.setTargetValue (spaceOn ? raw (ids::verbMix) : 0.0f);
    opto.setAmount (raw (ids::opto)); opto.setSpeed ((int) std::lround (raw (ids::optoSpd)));
    ott.set (raw (ids::ottOn) > 0.5f, raw (ids::ottAmt), raw (ids::ottDepth), raw (ids::ottTime), raw (ids::ottLo), raw (ids::ottMid), raw (ids::ottHi));
    dance.set (raw (ids::dmOn) > 0.5f, raw (ids::dmMix), raw (ids::dmDrive), raw (ids::dmClip), raw (ids::dmLow), raw (ids::dmMid), raw (ids::dmHigh));
    maximizer.setAmount (raw (ids::maxAmt));
    stereo.set (raw (ids::stOn) > 0.5f, raw (ids::stW), raw (ids::stWLo), raw (ids::stWHi), raw (ids::stMono), raw (ids::stMid), raw (ids::stSide), raw (ids::stMidDrv), raw (ids::stSideDrv));
    limiter.setEnabled (raw (ids::safety) > 0.5f);
    limiter.setCeiling (raw (ids::limCeil));
}

void PurrOneProcessor::applySeqSettings()
{
    auto raw = [this] (const char* id) { return rd (id); };
    SeqSettings c;
    c.mode = (int) std::lround (raw (ids::seqMode)); c.rate = (int) std::lround (raw (ids::seqRate)); c.syncHost = raw (ids::seqSync) < 0.5f;
    c.bpm = raw (ids::seqBpm); c.swing = raw (ids::seqSwing); c.gate = raw (ids::seqGate); c.human = raw (ids::seqHuman);
    c.transpose = (int) std::lround (raw (ids::seqTrans)); c.steps = (int) std::lround (raw (ids::seqSteps)); c.pattern = (int) std::lround (raw (ids::seqPat));
    c.arpPattern = (int) std::lround (raw (ids::arpPat)); c.arpOctaves = 1 + (int) std::lround (raw (ids::arpOct)); c.arpLatch = raw (ids::arpLatch) > 0.5f;
    c.startHost = raw (ids::seqOnHost) > 0.5f; c.startKey = raw (ids::seqOnKey) > 0.5f; c.startNote = raw (ids::seqOnNote) > 0.5f;
    c.triggerNote = (int) std::lround (raw (ids::seqTrigNote)); c.keysTranspose = raw (ids::seqKeyTrans) > 0.5f;
    sequencer.setSettings (c); sequencer.setManual (playButton.load());
    slideSeconds = raw (ids::seqSlide);
    midiOutOn = raw (ids::midiOut) > 0.5f; midiOutCh = (int) std::lround (raw (ids::midiCh)); midiOutVel = (int) std::lround (raw (ids::midiVel)); midiOutAcc = (int) std::lround (raw (ids::midiAcc));
}

void PurrOneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if ((int) mono.size() < n) return;                       // prepareToPlay not called with a big enough block; stay silent instead of allocating
    buffer.clear();
    if (uiBendNew.exchange (false)) { bendNorm = uiBend.load(); engine.setPitchBend (bendNorm * bendRange); bendShown.store (bendNorm); }
    keyState.processNextMidiBuffer (midi, 0, n, true);       // notes played on the editor keyboard
    applyParams(); applyFx(); applySeqSettings();

    HostTime ht;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            ht.playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) ht.bpm = *b;
            if (auto q = pos->getPpqPosition()) { ht.ppq = *q; ht.valid = true; }
        }
    const bool seqActive = sequencer.settings().mode != 0 || sequencer.isRunning();
    outMidi.clear();

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos) { engine.process (mono.data() + pos, end - pos); pos = end; }
    };
    auto sendOut = [&] (const SeqEvent& e)
    {
        if (! midiOutOn) return;
        const int ch = std::min (std::max (midiOutCh, 1), 16);
        if (e.type == SeqEvent::On)
        {
            outMidi.addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) std::min (std::max (e.accent ? midiOutAcc : midiOutVel, 1), 127)), e.at);
            if (lastOut >= 0 && lastOut != e.note) outMidi.addEvent (juce::MidiMessage::noteOff (ch, lastOut), e.at);   // slide: the old note ends after the new one starts
            else if (lastOut == e.note) outMidi.addEvent (juce::MidiMessage::noteOff (ch, lastOut), e.at);
            lastOut = e.note;
        }
        else if (lastOut >= 0) { outMidi.addEvent (juce::MidiMessage::noteOff (ch, lastOut), e.at); lastOut = -1; }
    };
    auto runSeq = [&] (int from, int to)
    {
        if (to <= from) return;
        HostTime h = ht; if (ht.valid && ht.playing && ht.bpm > 0.0) h.ppq = ht.ppq + (double) from * ht.bpm / 60.0 / sr;
        sequencer.run (seq, h, from, to - from);
        for (int i = 0; i < sequencer.numEvents(); ++i)
        {
            const auto& e = sequencer.event (i);
            renderTo (e.at);
            if (e.type == SeqEvent::On) { if (e.accent) accentCount.fetch_add (1, std::memory_order_relaxed); engine.seqNoteOn (e.note, e.accent, e.slide, slideSeconds); } else engine.seqNoteOff();
            sendOut (e);
        }
    };

    int segStart = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int at = std::min (meta.samplePosition, n);
        runSeq (segStart, at); segStart = at;
        renderTo (at);
        if (m.isNoteOn())
        {
            const int nn = m.getNoteNumber();
            sequencer.keyOn (nn);
            if (! sequencer.consumesKey (nn)) { engine.noteOn (nn, m.getFloatVelocity()); passed[nn] = true; }
        }
        else if (m.isNoteOff())
        {
            const int nn = m.getNoteNumber();
            sequencer.keyOff (nn);
            if (passed[nn]) { engine.noteOff (nn); passed[nn] = false; }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            engine.allNotesOff(); sequencer.allKeysOff(); std::fill (std::begin (passed), std::end (passed), false);
        }
        else if (m.isPitchWheel()) { bendNorm = (float) (m.getPitchWheelValue() - 8192) / 8192.0f; engine.setPitchBend (bendNorm * bendRange); bendShown.store (bendNorm); }
        else if (m.isController())
        {
            const int cc = m.getControllerNumber(); const int lt = learnTarget.load();
            if (lt >= 0 && cc > 0 && cc < 120 && cc != 32)              // MIDI learn: the next CC that moves is taken
            {
                for (auto& c : ccMap) if (c.load() == lt) c.store (-1);
                ccMap[cc].store (lt); learnTarget.store (-1);
            }
            if (cc == (wheelIsCC74 ? 74 : 1)) { midiWheel = (float) m.getControllerValue() / 127.0f; applyParams(); }
            const int idx = ccMap[cc & 127].load();
            if (idx >= 0 && idx < (int) learnParams.size())
            {
                learnParams[(size_t) idx]->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
                applyParams();
            }
        }
    }
    runSeq (segStart, n);
    renderTo (n);
    playStep.store (sequencer.currentStep()); seqRunning.store (sequencer.isRunning());

    if (midiOutOn && seqActive) { midi.clear(); midi.addEvents (outMidi, 0, n, 0); }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    float pl = 0.0f, pr = 0.0f;
    const float dAtk = 1.0f - std::exp (-1.0f / (0.003f * (float) sr)), dRel = 1.0f - std::exp (-1.0f / (0.15f * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        float x = crush.process (mono[(size_t) i]);
        if (eqPre) x = eq.processMono (x);
        float wl = 0.0f, wr = 0.0f;
        const float mixv = verbMixSm.getNextValue();
        if (spaceOn || mixv > 0.0005f) reverb.process (x, wl, wr);
        if (duckAmt > 0.001f)
        {
            const float a = std::fabs (x); duckEnv += (a > duckEnv ? dAtk : dRel) * (a - duckEnv);
            const float g = 1.0f - duckAmt * std::min (1.0f, duckEnv * 4.0f); wl *= g; wr *= g;
        }
        float l = x * (1.0f - mixv * 0.3f) + wl * mixv * 1.4f, r = x * (1.0f - mixv * 0.3f) + wr * mixv * 1.4f;
        if (eqPost) eq.processStereo (l, r);
        opto.process (l, r);
        ott.process (l, r);
        dance.process (l, r);
        maximizer.process (l, r);
        stereo.process (l, r);
        limiter.process (l, r);
        if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0.0f; engine.reset(); resetFx(); }
        L[i] = l; R[i] = r;
        { const int tp = tapPos.load (std::memory_order_relaxed); tapL[tp] = l; tapR[tp] = r; tapPos.store ((tp + 1) % kTap, std::memory_order_relaxed); }
        pl = std::max (pl, std::fabs (l)); pr = std::max (pr, std::fabs (r));
    }
    const float decay = std::pow (0.5f, (float) n / (float) (sr * 0.25));
    meterL.store (std::max (pl, meterL.load() * decay)); meterR.store (std::max (pr, meterR.load() * decay));
    if (std::max (pl, pr) >= 0.99f) clipped.store (true);
}

void PurrOneProcessor::resetFx()
{
    eq.prepare (sr); crush.prepare (sr); reverb.prepare (sr); opto.prepare (sr); ott.prepare (sr); dance.prepare (sr); maximizer.prepare (sr); stereo.prepare (sr);
    limiter.prepare (sr, rd (ids::limCeil)); duckEnv = 0.0f;
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
    seq.clear();
    if (list[(size_t) index].seq != nullptr && list[(size_t) index].seq[0] != 0) applySeqSpec (list[(size_t) index].seq);
    currentFactory = index; currentPresetName = list[(size_t) index].name;
}

juce::String PurrOneProcessor::sanitize (const juce::String& n)
{
    return juce::File::createLegalFileName (n.trim()).substring (0, 60);
}
static juce::File findUser (const juce::String& name)
{
    const auto fn = juce::File::createLegalFileName (name.trim()).substring (0, 60) + ".purrone";
    for (int b = 3; b <= 6; ++b) { auto f = PurrOneProcessor::userBankDir (b).getChildFile (fn); if (f.existsAsFile()) return f; }
    return PurrOneProcessor::userBankDir (3).getChildFile (fn);
}
juce::File PurrOneProcessor::userPresetDir()
{
    const auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    const auto dir = base.getChildFile ("Studio 87").getChildFile ("PurrOne").getChildFile ("presets");
    const auto old = base.getChildFile ("SpeakingColors").getChildFile ("PurrOne").getChildFile ("presets");   // pre-rebrand location
    if (! dir.isDirectory() && old.isDirectory())   // first run after the rebrand: copy (never move or delete) the user's old presets
    {
        dir.createDirectory();
        for (auto& f : old.findChildFiles (juce::File::findFiles, false, "*.purrone")) f.copyFileTo (dir.getChildFile (f.getFileName()));
    }
    return dir;
}
juce::StringArray PurrOneProcessor::listUserPresets() const
{
    juce::StringArray out;
    for (int b = 3; b <= 6; ++b) for (auto& n : listBank (b)) out.addIfNotAlreadyThere (n);
    out.sort (true); return out;
}
bool PurrOneProcessor::saveUserPreset (const juce::String& name)
{
    auto clean = sanitize (name); if (clean.isEmpty()) return false;
    if (saveBank < 3) return false;                                  // factory banks are read only
    auto dir = userBankDir (saveBank); if (! dir.createDirectory().wasOk()) return false;
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("presetName", clean); xml->setAttribute ("format", 1);
    if (rd (ids::seqBundle) > 0.5f) for (int i = 0; i < kSeqPatterns; ++i) xml->setAttribute ("seqP" + juce::String (i), patternToString (i));
    currentPresetName = clean;
    return xml->writeTo (dir.getChildFile (clean + ".purrone"));
}
bool PurrOneProcessor::renameUserPreset (const juce::String& from, const juce::String& to)
{
    auto cleanTo = sanitize (to); if (cleanTo.isEmpty()) return false;
    auto src = findUser (from);
    auto dst = src.getParentDirectory().getChildFile (cleanTo + ".purrone");
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
    auto file = findUser (name);
    if (! file.existsAsFile() || file.getSize() > 1024 * 256) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    // reset to defaults, then apply only known parameters, clamped by the parameter ranges
    for (auto* prm : getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) r->setValueNotifyingHost (r->getDefaultValue());
    for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
        if (auto* r = apvts.getParameter (child->getStringAttribute ("id")))
            r->setValueNotifyingHost (r->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
    if (xml->hasAttribute ("seqP0")) for (int i = 0; i < kSeqPatterns; ++i) patternFromString (i, xml->getStringAttribute ("seqP" + juce::String (i)));
    currentPresetName = name; return true;
}

// ---- state ----
void PurrOneProcessor::startLearn (const juce::String& id)
{
    for (size_t i = 0; i < learnParams.size(); ++i) if (learnParams[i]->paramID == id) { learnTarget.store ((int) i); return; }
}
int PurrOneProcessor::ccFor (const juce::String& id) const
{
    for (int c = 0; c < 128; ++c) { const int i = ccMap[c].load(); if (i >= 0 && i < (int) learnParams.size() && learnParams[(size_t) i]->paramID == id) return c; }
    return -1;
}
void PurrOneProcessor::forgetCC (const juce::String& id) { const int c = ccFor (id); if (c >= 0) ccMap[c].store (-1); }

void PurrOneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("format", 1); xml->setAttribute ("presetName", currentPresetName);
    for (int i = 0; i < kSeqPatterns; ++i) xml->setAttribute ("seqP" + juce::String (i), patternToString (i));
    { juce::String cm; for (int c = 0; c < 128; ++c) { const int i = ccMap[c].load(); if (i >= 0 && i < (int) learnParams.size()) cm << c << ":" << learnParams[(size_t) i]->paramID << ","; } xml->setAttribute ("ccmap", cm); }
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
    if (xml->hasAttribute ("ccmap"))
    {
        for (auto& c : ccMap) c.store (-1);
        for (const auto& item : juce::StringArray::fromTokens (xml->getStringAttribute ("ccmap").substring (0, 8000), ",", ""))
        {
            const int colon = item.indexOfChar (':'); if (colon < 1) continue;
            const int cc = item.substring (0, colon).getIntValue(); const auto id = item.substring (colon + 1);
            if (cc < 1 || cc > 127) continue;
            for (size_t i = 0; i < learnParams.size(); ++i) if (learnParams[i]->paramID == id) { ccMap[cc].store ((int) i); break; }
        }
    }
    if (xml->hasAttribute ("seqP0")) for (int i = 0; i < kSeqPatterns; ++i) patternFromString (i, xml->getStringAttribute ("seqP" + juce::String (i)));
}

// ---- sequence data ----
juce::String PurrOneProcessor::patternToString (int pat) const
{
    juce::String out; out.preallocateBytes (kSeqMaxSteps * 4 + 4);
    for (int i = 0; i < kSeqMaxSteps; ++i) out << juce::String::toHexString ((int) seq.get (pat, i)).paddedLeft ('0', 4);
    return out;
}
void PurrOneProcessor::patternFromString (int pat, const juce::String& s)
{
    const int w = s.length() > kSeqMaxSteps * 3 ? 4 : 3;                    // old saves use 3 hex digits per step, new ones 4 (chance and ratchet)
    for (int i = 0; i < kSeqMaxSteps; ++i)
    {
        const auto part = s.substring (i * w, i * w + w);
        const int v = part.length() == w ? part.getHexValue32() : 0;
        seq.set (pat, i, SeqData::pack (std::min (v & 63, 36), (v & 64) != 0, (v & 128) != 0, (v & 256) != 0, (v >> 9) & 3, (v >> 11) & 3));
    }
}
// Factory notation: space separated steps. "-" is a rest. A note is a name and octave, like "C2", "F#3", "Bb2" (C2 is the lowest, C5 the highest).
// Suffix "a" adds an accent and "s" a slide: "E2as". The number of steps becomes the sequence length.
void PurrOneProcessor::applySeqSpec (const juce::String& spec)
{
    auto tokens = juce::StringArray::fromTokens (spec, " ", "");
    tokens.removeEmptyStrings();
    for (int i = 0; i < kSeqMaxSteps; ++i) seq.set (0, i, SeqData::pack (0, false, false, false));
    int count = 0;
    for (auto& t : tokens)
    {
        if (count >= kSeqMaxSteps) break;
        if (t == "-" ) { seq.set (0, count++, SeqData::pack (0, false, false, false)); continue; }
        static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };               // A B C D E F G
        const int letter = (int) juce::CharacterFunctions::toUpperCase (t[0]) - 'A';
        if (letter < 0 || letter > 6) { seq.set (0, count++, SeqData::pack (0, false, false, false)); continue; }
        int semi = base[letter], k = 1;
        if (t[k] == '#') { ++semi; ++k; } else if (t[k] == 'b') { --semi; ++k; }
        const int octave = t[k] >= '0' && t[k] <= '9' ? (int) (t[k] - '0') : 2; if (t[k] >= '0' && t[k] <= '9') ++k;
        const bool acc = t.substring (k).containsChar ('a'), sl = t.substring (k).containsChar ('s');
        seq.set (0, count++, SeqData::pack ((octave - 2) * 12 + semi, true, acc, sl));
    }
    if (count > 0) setParam (ids::seqSteps, (float) count);
}

juce::File PurrOneProcessor::sequenceDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Studio 87").getChildFile ("PurrOne").getChildFile ("sequences");
}
juce::StringArray PurrOneProcessor::listSequences() const
{
    juce::StringArray out;
    for (auto& f : sequenceDir().findChildFiles (juce::File::findFiles, false, "*.purrseq")) out.add (f.getFileNameWithoutExtension());
    out.sort (true); return out;
}
static const char* const kSeqParamIds[] = { ids::seqRate, ids::seqSwing, ids::seqGate, ids::seqAcc, ids::seqSlide, ids::seqSteps, ids::squelch, ids::squelchAmt };
bool PurrOneProcessor::saveSequence (const juce::String& name)
{
    auto clean = sanitize (name); if (clean.isEmpty()) return false;
    auto dir = sequenceDir(); if (! dir.createDirectory().wasOk()) return false;
    juce::XmlElement xml ("PurrSeq"); xml.setAttribute ("name", clean); xml.setAttribute ("format", 1);
    xml.setAttribute ("pattern", patternToString ((int) std::lround (rd (ids::seqPat))));
    for (auto* id : kSeqParamIds) xml.setAttribute (id, (double) rd (id));
    return xml.writeTo (dir.getChildFile (clean + ".purrseq"));
}
bool PurrOneProcessor::loadSequence (const juce::String& name)
{
    auto file = sequenceDir().getChildFile (sanitize (name) + ".purrseq");
    if (! file.existsAsFile() || file.getSize() > 1024 * 64) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName ("PurrSeq")) return false;
    patternFromString ((int) std::lround (rd (ids::seqPat)), xml->getStringAttribute ("pattern"));
    for (auto* id : kSeqParamIds) if (xml->hasAttribute (id)) setParam (id, (float) xml->getDoubleAttribute (id));
    return true;
}

// ---- banks (A to C factory, D to G yours; D lives in the main folder so older saves stay where they were) ----
juce::File PurrOneProcessor::userBankDir (int bank)
{
    auto d = userPresetDir();
    return bank <= 3 ? d : d.getChildFile (juce::String ("Bank ") + juce::String::charToString ((juce::juce_wchar) ('A' + juce::jlimit (4, 6, bank))));
}
juce::StringArray PurrOneProcessor::listBank (int bank) const
{
    juce::StringArray out;
    for (auto& f : userBankDir (bank).findChildFiles (juce::File::findFiles, false, "*.purrone")) out.add (f.getFileNameWithoutExtension());
    out.sort (true); return out;
}
bool PurrOneProcessor::exportBank (int bank, const juce::File& f) const
{
    juce::XmlElement root ("PurrBank"); root.setAttribute ("format", 1); root.setAttribute ("bank", juce::String::charToString ((juce::juce_wchar) ('A' + bank)));
    int n = 0;
    for (auto& file : userBankDir (bank).findChildFiles (juce::File::findFiles, false, "*.purrone"))
        if (n < 128) if (auto xml = juce::XmlDocument::parse (file)) { root.addChildElement (xml.release()); ++n; }
    return n > 0 && root.writeTo (f);
}
int PurrOneProcessor::importBank (const juce::File& f, int bank)
{
    if (! f.existsAsFile() || f.getSize() > 1024 * 1024 * 8) return -1;
    std::unique_ptr<juce::XmlElement> root (juce::XmlDocument::parse (f));
    if (root == nullptr || ! root->hasTagName ("PurrBank")) return -1;
    auto dir = userBankDir (bank); if (! dir.createDirectory().wasOk()) return -1;
    int n = listBank (bank).size(), added = 0;
    for (auto* child : root->getChildIterator())
    {
        if (n >= 128) break;
        if (! child->hasTagName (apvts.state.getType())) continue;
        const auto nm = sanitize (child->getStringAttribute ("presetName", "Imported")); if (nm.isEmpty()) continue;
        auto dst = dir.getChildFile (nm + ".purrone"); if (dst.existsAsFile()) continue;
        if (child->writeTo (dst)) { ++n; ++added; }
    }
    return added;
}
bool PurrOneProcessor::exportSound (const juce::File& f)
{
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("presetName", currentPresetName); xml->setAttribute ("format", 1);
    if (rd (ids::seqBundle) > 0.5f) for (int i = 0; i < kSeqPatterns; ++i) xml->setAttribute ("seqP" + juce::String (i), patternToString (i));
    return xml->writeTo (f);
}
bool PurrOneProcessor::importSound (const juce::File& f, int bank)
{
    if (! f.existsAsFile() || f.getSize() > 1024 * 256 || listBank (bank).size() >= 128) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (f));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    auto nm = sanitize (xml->getStringAttribute ("presetName", f.getFileNameWithoutExtension())); if (nm.isEmpty()) nm = "Imported";
    auto dir = userBankDir (bank); if (! dir.createDirectory().wasOk()) return false;
    return xml->writeTo (dir.getChildFile (nm + ".purrone"));
}
bool PurrOneProcessor::deleteUserPreset (const juce::String& name)
{
    bool any = false;
    for (int b = 3; b <= 6; ++b) { auto f = userBankDir (b).getChildFile (sanitize (name) + ".purrone"); if (f.existsAsFile()) any = f.moveToTrash() || f.deleteFile() || any; }
    return any;
}
bool PurrOneProcessor::deleteSequence (const juce::String& name)
{
    auto f = sequenceDir().getChildFile (sanitize (name) + ".purrseq");
    return f.existsAsFile() && (f.moveToTrash() || f.deleteFile());
}
juce::String PurrOneProcessor::sequenceCode (const juce::String& name)
{
    juce::XmlElement xml ("PurrSeq"); xml.setAttribute ("name", sanitize (name)); xml.setAttribute ("format", 1);
    xml.setAttribute ("pattern", patternToString ((int) std::lround (rd (ids::seqPat))));
    for (auto* id : kSeqParamIds) xml.setAttribute (id, (double) rd (id));
    return "PO-SEQ:" + juce::Base64::toBase64 (xml.toString (juce::XmlElement::TextFormat().singleLine()));
}
bool PurrOneProcessor::importSequenceCode (const juce::String& code, const juce::String& name)
{
    auto c = code.trim(); if (! c.startsWith ("PO-SEQ:") || c.length() > 40000) return false;
    juce::MemoryOutputStream out;
    if (! juce::Base64::convertFromBase64 (out, c.substring (7).trim())) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (out.toString()));
    if (xml == nullptr || ! xml->hasTagName ("PurrSeq")) return false;
    auto nm = sanitize (name.isNotEmpty() ? name : xml->getStringAttribute ("name", "Shared")); if (nm.isEmpty()) return false;
    xml->setAttribute ("name", nm);
    auto dir = sequenceDir(); if (! dir.createDirectory().wasOk()) return false;
    return xml->writeTo (dir.getChildFile (nm + ".purrseq"));
}

void PurrOneProcessor::setParam (const char* id, float v)
{
    if (auto* r = apvts.getParameter (id)) r->setValueNotifyingHost (r->convertTo0to1 (v));
}
void PurrOneProcessor::applyValues (const std::vector<std::pair<const char*, float>>& v)
{
    for (auto& kv : v) setParam (kv.first, kv.second);
}

juce::AudioProcessorEditor* PurrOneProcessor::createEditor() { return new PurrOneEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PurrOneProcessor(); }
