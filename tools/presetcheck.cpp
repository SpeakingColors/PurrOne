// Offline check of the real processor: dumps parameter defaults and renders every factory preset to find silence, clipping,
// NaN and loudness differences. Usage:  PurrOneCheck defaults > defaults.txt
//                                       PurrOneCheck check [targetRmsDb] > report.txt
#include "../src/PluginProcessor.h"
#include "../src/Presets.h"
#include <chrono>
#include <cstdio>

static juce::String s (const char* c) { return juce::String (c); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::String mode = argc > 1 ? argv[1] : "check";
    PurrOneProcessor p;
    if (mode == "defaults")
    {
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            {
                const auto range = r->getNormalisableRange();
                const char* type = dynamic_cast<juce::AudioParameterChoice*> (prm) ? "choice" : (dynamic_cast<juce::AudioParameterBool*> (prm) ? "bool" : (dynamic_cast<juce::AudioParameterInt*> (prm) ? "int" : "float"));
                std::printf ("%s %s %.6f %.6f %.6f\n", r->paramID.toRawUTF8(), type, (double) r->convertFrom0to1 (r->getDefaultValue()), (double) range.start, (double) range.end);
            }
        return 0;
    }
    if (mode == "learntest")
    {
        int fails = 0; auto ck = [&] (bool ok, const char* what) { std::printf ("%s %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++fails; };
        auto run = [] (PurrOneProcessor& q, int cc, int v) { juce::AudioBuffer<float> b (2, 256); b.clear(); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, cc, v), 0); q.processBlock (b, m); };
        p.prepareToPlay (48000.0, 256);
        auto val = [] (PurrOneProcessor& q) { return q.apvts.getRawParameterValue ("glide")->load(); };
        run (p, 20, 127); ck (std::fabs (val (p)) < 0.001f, "unmapped CC does nothing");
        p.startLearn ("glide"); ck (p.learningId() == "glide", "learn armed");
        run (p, 20, 10); ck (p.ccFor ("glide") == 20 && p.learningId().isEmpty(), "first CC is taken");
        run (p, 20, 127); ck (std::fabs (val (p) - 1.0f) < 0.01f, "CC 127 sets the top");
        run (p, 20, 0); ck (std::fabs (val (p)) < 0.01f, "CC 0 sets the bottom");
        juce::MemoryBlock mb; p.getStateInformation (mb);
        PurrOneProcessor q; q.prepareToPlay (48000.0, 256); q.setStateInformation (mb.getData(), (int) mb.getSize());
        ck (q.ccFor ("glide") == 20, "mapping survives save and load");
        run (q, 20, 64); ck (std::fabs (val (q) - 0.504f) < 0.02f, "loaded mapping moves the knob");
        p.startLearn ("glide"); run (p, 21, 5); ck (p.ccFor ("glide") == 21, "re-learn moves it to the new CC");
        p.forgetCC ("glide"); ck (p.ccFor ("glide") == -1, "forget clears it");
        { // sequencer step format: chance and ratchet survive state, and old 3-digit saves still load
            p.seq.set (0, 3, purrone::SeqData::pack (7, true, true, false, 2, 3)); p.seq.set (0, 9, purrone::SeqData::pack (12, true, false, true, 3, 1));
            juce::MemoryBlock m2; p.getStateInformation (m2); PurrOneProcessor r; r.prepareToPlay (48000.0, 256); r.setStateInformation (m2.getData(), (int) m2.getSize());
            const auto a3 = r.seq.get (0, 3), a9 = r.seq.get (0, 9);
            ck (purrone::SeqData::note (a3) == 7 && purrone::SeqData::gate (a3) && purrone::SeqData::accent (a3) && purrone::SeqData::chance (a3) == 2 && purrone::SeqData::ratchet (a3) == 4, "step chance and ratchet survive state");
            ck (purrone::SeqData::note (a9) == 12 && purrone::SeqData::slide (a9) && purrone::SeqData::chance (a9) == 3 && purrone::SeqData::ratchet (a9) == 2, "second step too");
            juce::String old; for (int i = 0; i < 64; ++i) old << juce::String::toHexString (i == 2 ? 0xC5 : 0).paddedLeft ('0', 3);   // note 5, gate, accent
            r.patternFromString (0, old); const auto o2 = r.seq.get (0, 2);
            ck (purrone::SeqData::note (o2) == 5 && purrone::SeqData::gate (o2) && purrone::SeqData::accent (o2) && purrone::SeqData::chance (o2) == 0 && purrone::SeqData::ratchet (o2) == 1, "old 3-digit pattern still loads");
        }
        { // performance routing: wheel and aftertouch to cutoff, vibrato and reverb
            auto render = [&] (const char* id1, float v1, const char* id2, float v2, int cc, int ccv, bool pressure, int blocks, int noteOffAt, std::vector<float>& out)
            {
                PurrOneProcessor q; q.prepareToPlay (48000.0, 256); q.setParam ("cutoff", 500.0f); q.setParam ("reso", 0.1f); q.setParam ("vol", 0.8f);
                if (id1) q.setParam (id1, v1); if (id2) q.setParam (id2, v2);
                for (int b = 0; b < blocks; ++b)
                {
                    juce::AudioBuffer<float> buf (2, 256); buf.clear(); juce::MidiBuffer m;
                    if (b == 0) { m.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0); if (pressure) m.addEvent (juce::MidiMessage::channelPressureChange (1, ccv), 1); else if (cc >= 0) m.addEvent (juce::MidiMessage::controllerEvent (1, cc, ccv), 1); }
                    if (b == noteOffAt) m.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
                    q.processBlock (buf, m); for (int i = 0; i < 256; ++i) out.push_back (buf.getSample (0, i));
                }
            };
            auto rmsOf = [] (const std::vector<float>& v, size_t a, size_t b) { double e = 0; for (size_t i = a; i < b && i < v.size(); ++i) e += (double) v[i] * v[i]; return 10.0 * std::log10 (e / (double) (b - a) + 1e-20); };
            std::vector<float> base, wh, at, nowheel; char d[160];
            render (nullptr, 0, nullptr, 0, 1, 127, false, 120, 1000, base);
            render ("wheelCut", 1.0f, nullptr, 0, 1, 127, false, 120, 1000, wh);
            render ("atCut", 1.0f, nullptr, 0, -1, 127, true, 120, 1000, at);
            render ("wheelCut", 1.0f, nullptr, 0, -1, 0, false, 120, 1000, nowheel);
            auto bright = [&] (const std::vector<float>& v) { std::vector<float> df; for (size_t i = 15001; i < 30000 && i < v.size(); ++i) df.push_back (v[i] - v[i - 1]); return rmsOf (df, 0, df.size()); };   // high-frequency energy
            std::snprintf (d, sizeof d, "brightness base %.1f dB, wheel open %.1f dB", bright (base), bright (wh));
            ck (bright (wh) > bright (base) + 3.0, d);   // the Init sound is dull, a fully open filter adds about 4 dB here
            std::snprintf (d, sizeof d, "aftertouch open %.1f dB", bright (at)); ck (bright (at) > bright (base) + 3.0, d);
            double dev = 0; for (size_t i = 0; i < base.size(); ++i) dev = std::max (dev, (double) std::fabs (base[i] - nowheel[i]));
            ck (dev < 1e-4, "wheel at zero changes nothing");
            std::vector<float> v0, v1;
            render (nullptr, 0, nullptr, 0, 1, 127, false, 120, 1000, v0); render ("wheelVib", 1.0f, "lfoHz", 6.0f, 1, 127, false, 120, 1000, v1);
            double dv = 0; for (size_t i = 15000; i < v0.size() && i < v1.size(); ++i) dv += std::fabs (v0[i] - v1[i]); bool fin = true; for (float x : v1) fin = fin && std::isfinite (x);
            ck (dv > 1.0 && fin, "wheel vibrato changes the pitch and stays finite");
            std::vector<float> r0, r1; render (nullptr, 0, nullptr, 0, 1, 127, false, 160, 40, r0); render ("wheelVerb", 1.0f, nullptr, 0, 1, 127, false, 160, 40, r1);
            std::snprintf (d, sizeof d, "tail without %.1f dB, with %.1f dB", rmsOf (r0, 20000, 40000), rmsOf (r1, 20000, 40000));
            ck (rmsOf (r1, 20000, 40000) > rmsOf (r0, 20000, 40000) + 3.0, d);
        }
        std::printf ("%d failures\n", fails); return fails != 0;
    }
    const double target = argc > 2 ? std::atof (argv[2]) : -18.0;
    const double sr = 48000.0; const int block = 256, total = (int) (sr * 4.0);
    const int n = p.getNumPrograms();
    double worstCpu = 0.0;
    int bad = 0;
    for (int idx = 0; idx < n; ++idx)
    {
        p.setCurrentProgram (idx);
        p.prepareToPlay (sr, block);
        const auto* f = &purrone::factoryPresets()[(size_t) idx];
        const int mode_ = (int) std::lround (p.apvts.getRawParameterValue ("seqMode")->load());
        juce::String cat (f->category);
        std::vector<int> notes;
        if (mode_ == 2) notes = { 48, 52, 55 };
        else if (mode_ == 1) notes = { 36 };
        else if (cat == "Lead") notes = { 60 };
        else if (cat == "FX") notes = { 48 };
        else notes = { 36 };
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer midi;
        double sum = 0.0; long cnt = 0; float peak = 0.0f; double dc = 0.0; bool finite = true;
        // loudness = RMS over the blocks that are actually sounding (above -50 dB), so sparse sequences and one shots compare fairly
        const int winFrom = (int) (sr * 0.05), winTo = (int) (sr * (mode_ == 0 ? 2.4 : 3.9));
        auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos < total; pos += block)
        {
            midi.clear();
            if (pos == 0) for (int k : notes) midi.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) 100), 0);
            if (mode_ == 0 && pos == (int) (sr * 2.5) / block * block) for (int k : notes) midi.addEvent (juce::MidiMessage::noteOff (1, k), 0);
            buf.clear(); p.processBlock (buf, midi);
            double blk = 0.0;
            for (int i = 0; i < block; ++i)
            {
                const float l = buf.getSample (0, i), r = buf.getSample (1, i);
                if (! std::isfinite (l) || ! std::isfinite (r)) finite = false;
                blk += (double) l * l + (double) r * r; dc += l;
                peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
            }
            if (pos >= winFrom && pos < winTo && 10.0 * std::log10 (blk / (2.0 * block) + 1e-12) > -50.0) { sum += blk; cnt += 2L * block; }
        }
        const double cpu = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / 4.0;
        worstCpu = std::max (worstCpu, cpu);
        const double rms = 10.0 * std::log10 (sum / (double) std::max (1L, cnt) + 1e-12);
        dc /= total;
        const double vol = p.apvts.getRawParameterValue ("vol")->load();
        const double want = std::min (1.0, std::max (0.05, vol * std::pow (10.0, (target - rms) / 20.0)));
        const bool silent = cnt == 0 || rms < -50.0;
        if (! finite || silent || peak > 0.95f) ++bad;
        std::printf ("%d\t%s\t%s\t%.1f\t%.1f\t%.4f\t%.3f\t%.3f\t%s%s%s\n", idx, f->name, f->category, rms, 20.0 * std::log10 (peak + 1e-9), dc, vol, want, finite ? "" : "NAN ", silent ? "SILENT " : "", peak > 0.95f ? "CLIP" : "");
        std::fflush (stdout);
    }
    std::fprintf (stderr, "%d presets, %d with problems, worst cpu %.1f%% of one core\n", n, bad, worstCpu * 100.0);
    return bad ? 1 : 0;
}
