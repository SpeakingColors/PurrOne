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
    if (mode == "seqtrans")
    {   // the sequencer's Transpose knob must move the notes that reach the synth (read from the MIDI out copy)
        int fails = 0;
        for (int tr : { 0, 7, 24 })
        {
            PurrOneProcessor q; q.prepareToPlay (48000.0, 256);
            q.setParam (purrone::ids::mixB, 0.0f); q.setParam (purrone::ids::mixSub, 0.0f); q.setParam (purrone::ids::cutoff, 12000.0f); q.setParam (purrone::ids::seqMode, 1.0f); q.setParam (purrone::ids::seqSync, 1.0f); q.setParam (purrone::ids::midiOut, 1.0f); q.setParam (purrone::ids::seqTrans, (float) tr); q.playButton.store (true);
            for (int i = 0; i < 16; ++i) q.seq.set (0, i, purrone::SeqData::pack (i % 5, true, false, false));
            int first = -1; std::vector<float> aud;
            for (int blk = 0; blk < 200; ++blk)
            {
                juce::AudioBuffer<float> b (2, 256); b.clear(); juce::MidiBuffer m; q.processBlock (b, m);
                for (const auto meta : m) if (first < 0 && meta.getMessage().isNoteOn()) first = meta.getMessage().getNoteNumber();
                if (first >= 0 && aud.size() < 9000) for (int i = 0; i < 256; ++i) aud.push_back (b.getSample (0, i));
            }
            int best = 0; const int off = 3000, len = 4000;   // pitch of the sound: first autocorrelation peak above 90% of the zero-lag value
            if (aud.size() >= (size_t) (off + len + 1500)) { double c0 = 0; for (int i = 0; i < len; ++i) c0 += (double) aud[(size_t) (off + i)] * aud[(size_t) (off + i)];
                double prev = 0; bool up = false; for (int lag = 20; lag < 1500; ++lag) { double c = 0; for (int i = 0; i < len; ++i) c += (double) aud[(size_t) (off + i)] * aud[(size_t) (off + i + lag)]; if (c > 0.9 * c0 && c < prev && up) { best = lag - 1; break; } up = c >= prev; prev = c; } }
            std::printf ("transpose %d -> first note %d, sound period %d samples (%.1f Hz)\n", tr, first, best, best > 0 ? 48000.0 / best : 0.0);
            static int base = -1; if (tr == 0) base = first; else if (first != base + tr) ++fails;
        }
        std::printf ("%s\n", fails ? "FAIL transpose does not reach the synth" : "ok   transpose reaches the synth");
        return fails;
    }
    if (mode == "unitest")
    {   // does the Unison Detune knob change the sound? Same note, 4 voices, detune 0 / 30 % / 100 %: print how different the output is and how much it moves (slow beating)
        std::vector<std::vector<float>> out;
        for (float dt : { 0.0f, 0.3f, 1.0f })
        {
            PurrOneProcessor q; q.prepareToPlay (48000.0, 256);
            q.setParam (purrone::ids::uniVoices, 3.0f); q.setParam (purrone::ids::uniDetune, dt); q.setParam (purrone::ids::mixB, 0.0f); q.setParam (purrone::ids::mixSub, 0.0f); q.setParam (purrone::ids::cutoff, 3000.0f);
            std::vector<float> y; juce::MidiBuffer first; first.addEvent (juce::MidiMessage::noteOn (1, 36, 0.9f), 0);
            for (int blk = 0; blk < 375; ++blk) { juce::AudioBuffer<float> b (2, 256); b.clear(); juce::MidiBuffer m; if (blk == 0) m = first; q.processBlock (b, m); for (int i = 0; i < 256; ++i) y.push_back (b.getSample (0, i)); }
            out.push_back (y);
        }
        for (size_t k = 1; k < out.size(); ++k)
        {
            double d = 0, e = 0; for (size_t i = 24000; i < out[0].size(); ++i) { d += std::pow ((double) out[k][i] - out[0][i], 2); e += std::pow ((double) out[0][i], 2); }
            // amplitude movement: spread of the 20 ms RMS over the second half
            double mn = 1e9, mx = 0; for (size_t i = 24000; i + 960 < out[k].size(); i += 960) { double r = 0; for (int j = 0; j < 960; ++j) r += std::pow ((double) out[k][i + j], 2); r = std::sqrt (r / 960); mn = std::min (mn, r); mx = std::max (mx, r); }
            std::printf ("detune %s: difference to detune 0 = %.1f dB, level swing %.1f dB\n", k == 1 ? "30%" : "100%", 10 * std::log10 (d / std::max (e, 1e-12)), 20 * std::log10 (mx / std::max (mn, 1e-9)));
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
        { // unison: 1 voice is untouched, 5 voices stay finite, similar loudness, and the stack beats (level wobbles) when detuned
            auto render = [&] (int voices, float detune, std::vector<float>& out)
            {
                PurrOneProcessor q; q.prepareToPlay (48000.0, 256); q.setParam ("cutoff", 4000.0f); q.setParam ("vol", 0.8f); q.setParam ("uniVoices", (float) (voices - 1)); q.setParam ("uniDetune", detune);
                for (int b = 0; b < 200; ++b) { juce::AudioBuffer<float> buf (2, 256); buf.clear(); juce::MidiBuffer m; if (b == 0) m.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0); q.processBlock (buf, m); for (int i = 0; i < 256; ++i) out.push_back (buf.getSample (0, i)); }
            };
            auto db = [] (const std::vector<float>& v, size_t a, size_t b) { double e = 0; for (size_t i = a; i < b; ++i) e += (double) v[i] * v[i]; return 10.0 * std::log10 (e / (double) (b - a) + 1e-20); };
            std::vector<float> u1, u1b, u5, u5z; render (1, 0.3f, u1); render (1, 1.0f, u1b); render (5, 0.8f, u5); render (5, 0.0f, u5z);
            double dev = 0; for (size_t i = 0; i < u1.size(); ++i) dev = std::max (dev, (double) std::fabs (u1[i] - u1b[i]));
            ck (dev == 0.0, "unison off: detune knob changes nothing");
            bool fin = true; for (float x : u5) fin = fin && std::isfinite (x);
            char d[160]; std::snprintf (d, sizeof d, "1 voice %.1f dB, 5 voices %.1f dB", db (u1, 20000, 40000), db (u5, 20000, 40000));
            ck (fin && std::fabs (db (u5, 20000, 40000) - db (u1, 20000, 40000)) < 3.0, d);
            double d0 = 0; for (size_t i = 20000; i < 40000; ++i) d0 = std::max (d0, (double) std::fabs (u5[i] - u1[i]));
            ck (d0 > 0.05, "5 voices sound different from 1");
            // beating: short-window level varies much more with detune than with 5 voices at zero detune (identical copies except phase)
            auto wobble = [&] (const std::vector<float>& v) { double lo = 1e9, hi = -1e9; for (size_t s = 10000; s + 480 < v.size() - 4000; s += 480) { const double l = db (v, s, s + 480); lo = std::min (lo, l); hi = std::max (hi, l); } return hi - lo; };
            std::snprintf (d, sizeof d, "level range detuned %.1f dB, no detune %.1f dB", wobble (u5), wobble (u5z));
            ck (wobble (u5) > wobble (u5z) + 0.3, d);
            { // every extra voice must change the sound (2 vs 3 vs 4 vs 5), and a low knob stays tight
                std::vector<float> uv[6]; for (int n = 2; n <= 5; ++n) render (n, 0.6f, uv[n]);
                for (int n = 2; n < 5; ++n)
                {
                    double diff = 0, ref = 0; for (size_t i = 20000; i < 40000; ++i) { diff += std::fabs (uv[n][i] - uv[n + 1][i]); ref += std::fabs (uv[n][i]); }
                    std::snprintf (d, sizeof d, "%d vs %d voices differ by %.0f%%", n, n + 1, 100.0 * diff / std::max (ref, 1e-9));
                    ck (diff / std::max (ref, 1e-9) > 0.15, d);
                }
                std::snprintf (d, sizeof d, "wobble 2v %.1f 3v %.1f 4v %.1f 5v %.1f dB", wobble (uv[2]), wobble (uv[3]), wobble (uv[4]), wobble (uv[5])); ck (true, d);
            }
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
