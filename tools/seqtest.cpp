// Offline checks for the 0.2 additions: new FX, sequencer / arp timing, squelch, 8x oversampling, neutral defaults. No JUCE.
#include "purrone/Engine.h"
#include "purrone/Fx2.h"
#include "purrone/Seq.h"
#include <cstdio>
#include <vector>
using namespace purrone;
static double sr = 48000;
static int fails = 0;
static void check (const char* name, bool ok, const char* detail) { printf ("%-44s %s  %s\n", name, ok ? "PASS" : "FAIL", detail); if (! ok) ++fails; }
static float sine (int n, double f, double a) { return (float) (a * std::sin (kTwoPi * f * n / sr)); }
static double rmsDb (const std::vector<float>& x, int from) { double s = 0; for (size_t i = (size_t) from; i < x.size(); ++i) s += (double) x[i] * x[i]; return 10 * std::log10 (s / (double) (x.size() - (size_t) from) + 1e-20); }

int main()
{
    char d[200];
    { // EQ: stereo-target bell +6 dB at 1 kHz; mono signal is untouched by a Side band; Mid band acts on a mono signal
        Eq5 eq; eq.prepare (sr);
        for (int i = 0; i < 5; ++i) eq.setBand (i, true, 1000.0f, i == 2 ? 6.0f : 0.0f, 0.8f, 0.0f, -24.0f, 0);
        std::vector<float> y; for (int n = 0; n < 48000; ++n) y.push_back (eq.processMono (sine (n, 1000, 0.25)));
        double g = rmsDb (y, 24000) - 10 * std::log10 (0.25 * 0.25 / 2);
        std::snprintf (d, sizeof d, "measured %+.2f dB", g); check ("eq5 bell +6 dB", std::fabs (g - 6.0) < 0.3, d);

        Eq5 es; es.prepare (sr); for (int i = 0; i < 5; ++i) es.setBand (i, true, 1000.0f, i == 2 ? 6.0f : 0.0f, 0.8f, 0.0f, -24.0f, i == 2 ? 2 : 0);
        double dev = 0; for (int n = 0; n < 4800; ++n) { float v = sine (n, 1000, 0.25), l = v, r = v; es.processStereo (l, r); dev = std::max (dev, (double) std::fabs (l - v)); }
        check ("eq5 side band leaves mono alone", dev < 1e-6, "mono in, mono out");
        Eq5 em; em.prepare (sr); for (int i = 0; i < 5; ++i) em.setBand (i, true, 1000.0f, i == 2 ? 6.0f : 0.0f, 0.8f, 0.0f, -24.0f, i == 2 ? 1 : 0);
        std::vector<float> ym; for (int n = 0; n < 48000; ++n) { float v = sine (n, 1000, 0.25), l = v, r = v; em.processStereo (l, r); ym.push_back (l); }
        g = rmsDb (ym, 24000) - 10 * std::log10 (0.25 * 0.25 / 2);
        std::snprintf (d, sizeof d, "measured %+.2f dB", g); check ("eq5 mid band boosts mono signal", std::fabs (g - 6.0) < 0.3, d);

        // dynamic is added to the static gain in either direction: static -3 dB and dynamic -6 dB gives -3 quiet and -9 loud; static 0 and dynamic +6 gives 0 quiet and +6 loud
        auto dyn = [&] (double amp, float sg, float dg) { Eq5 e; e.prepare (sr); for (int i = 0; i < 5; ++i) e.setBand (i, true, 1000.0f, i == 2 ? sg : 0.0f, 0.8f, i == 2 ? dg : 0.0f, -30.0f, 0);
            std::vector<float> o; for (int n = 0; n < 96000; ++n) o.push_back (e.processMono (sine (n, 1000, amp))); return rmsDb (o, 48000) - 10 * std::log10 (amp * amp / 2); };
        double q = dyn (0.003, -3.0f, -6.0f), lo = dyn (0.5, -3.0f, -6.0f), qb = dyn (0.003, 0.0f, 6.0f), lb = dyn (0.5, 0.0f, 6.0f);
        std::snprintf (d, sizeof d, "quiet %+.2f dB, loud %+.2f dB", q, lo); check ("eq5 dynamic cuts more when loud", lo < q - 5.0 && std::fabs (q + 3.0) < 1.0 && std::fabs (lo + 9.0) < 1.0, d);
        std::snprintf (d, sizeof d, "quiet %+.2f dB, loud %+.2f dB", qb, lb); check ("eq5 dynamic boosts when loud (no static gain)", std::fabs (qb) < 0.5 && std::fabs (lb - 6.0) < 1.0, d);
    }
    { // the default EQ (low 0, mid +2.3, high -3) must sound exactly like the old 3 band EQ
        Eq5 e5; e5.prepare (sr); Eq3 e3; e3.prepare (sr); e3.setGains (0.0, 2.3, -3.0);
        e5.setBand (0, true, 100.0f, 0.0f, 0.7071f, 0.0f, -24.0f, 0); e5.setBand (1, true, 350.0f, 0.0f, 1.0f, 0.0f, -24.0f, 0); e5.setBand (2, true, 1000.0f, 2.3f, 0.8f, 0.0f, -24.0f, 0);
        e5.setBand (3, true, 3200.0f, 0.0f, 1.0f, 0.0f, -24.0f, 0); e5.setBand (4, true, 6000.0f, -3.0f, 0.7071f, 0.0f, -24.0f, 0);
        double dev = 0; for (int n = 0; n < 48000; ++n) { float v = sine (n, 80, 0.2) + sine (n, 1300, 0.2) + sine (n, 9000, 0.1); dev = std::max (dev, (double) std::fabs (e5.processMono (v) - e3.process (v))); }
        std::snprintf (d, sizeof d, "max difference %.2e", dev); check ("default eq equals the old eq", dev < 1e-6, d);
    }
    { // crush: neutral is exact; 4 bit changes the signal
        Crush c; c.prepare (sr); c.set (true, 16.0f, 48000.0f, 0.5f); bool same = true; for (int n = 0; n < 1000; ++n) { float v = sine (n, 440, 0.4); same = same && c.process (v) == v; }
        check ("crush neutral is exact", same, "bits 16, rate 48k");
        c.set (true, 4.0f, 12000.0f, 1.0f); double dev = 0; for (int n = 0; n < 4800; ++n) { float v = sine (n, 440, 0.4); dev = std::max (dev, (double) std::fabs (c.process (v) - v)); }
        std::snprintf (d, sizeof d, "max change %.3f", dev); check ("crush 4 bit changes the signal", dev > 0.02, d);
    }
    { // stereo tool: neutral exact; width 0 gives mono; mono below removes low side
        StereoTool s; s.prepare (sr); s.set (true, 1, 1, 1, 20, 0, 0, 0, 0); bool same = true;
        for (int n = 0; n < 1000; ++n) { float l = sine (n, 300, 0.3), r = sine (n, 500, 0.3), l0 = l, r0 = r; s.process (l, r); same = same && std::fabs (l - l0) < 1e-6f && std::fabs (r - r0) < 1e-6f; }
        check ("stereo tool neutral is exact", same, "all defaults");
        StereoTool w; w.prepare (sr); w.set (true, 0, 1, 1, 20, 0, 0, 0, 0); double dev = 0;
        for (int n = 0; n < 9600; ++n) { float l = sine (n, 300, 0.3), r = sine (n, 500, 0.3); w.process (l, r); if (n > 2000) dev = std::max (dev, (double) std::fabs (l - r)); }
        std::snprintf (d, sizeof d, "|L-R| max %.5f", dev); check ("stereo width 0 is mono", dev < 1e-4, d);
        StereoTool m; m.prepare (sr); m.set (true, 1, 1, 1, 200, 0, 0, 0, 0); std::vector<float> sl, sh;
        for (int n = 0; n < 48000; ++n) { float v = sine (n, 60, 0.3), l = v, r = -v; m.process (l, r); sl.push_back (0.5f * (l - r)); }
        for (int n = 0; n < 48000; ++n) { float v = sine (n, 2000, 0.3), l = v, r = -v; m.process (l, r); sh.push_back (0.5f * (l - r)); }
        std::snprintf (d, sizeof d, "60 Hz side %+.1f dB, 2 kHz side %+.1f dB", rmsDb (sl, 24000), rmsDb (sh, 24000));
        check ("mono below removes low side only", rmsDb (sl, 24000) < -40.0 && rmsDb (sh, 24000) > -15.0, d);
    }
    { // OTT and Dance master: finite, off is transparent, on narrows level spread
        Ott o; o.prepare (sr); o.set (false, 0.5f, 0.6f, 0.4f, 0, 0, 0); float l = 0.3f, r = 0.3f; o.process (l, r);
        check ("ott off is transparent", l == 0.3f && r == 0.3f, "exact");
        auto spread = [&] (double amp) { Ott t; t.prepare (sr); t.set (true, 1.0f, 0.8f, 0.4f, 0, 0, 0); std::vector<float> y; bool fin = true;
            for (int n = 0; n < 96000; ++n) { float a = sine (n, 220, amp), b = a; t.process (a, b); fin = fin && std::isfinite (a); y.push_back (a); } return fin ? rmsDb (y, 48000) : 99.0; };
        double lo = spread (0.01), hi = spread (0.5); double inSp = 20 * std::log10 (0.5 / 0.01);
        std::snprintf (d, sizeof d, "level spread in %.1f dB, out %.1f dB", inSp, hi - lo); check ("ott narrows level spread", hi - lo < inSp - 10.0, d);
        DanceMaster dm; dm.prepare (sr); dm.set (false, 0.5f, 0.3f, 0.3f, 0, 0.5f, 0.4f); l = r = 0.3f; dm.process (l, r);
        check ("dance master off is transparent", l == 0.3f && r == 0.3f, "exact");
        DanceMaster dn; dn.prepare (sr); dn.set (true, 1.0f, 0.5f, 0.6f, 0.0f, 0.8f, 0.8f); bool fin = true; float pk = 0; std::vector<float> y;
        for (int n = 0; n < 96000; ++n) { float a = sine (n, 80, 0.9) + sine (n, 3000, 0.3), b = a; dn.process (a, b); fin = fin && std::isfinite (a); pk = std::max (pk, std::fabs (a)); y.push_back (a); }
        std::snprintf (d, sizeof d, "peak %.2f", pk); check ("dance master finite and bounded", fin && pk < 2.5f, d);
    }
    { // reverb extras: defaults identical to the classic settings, pre-delay moves the first sound
        Reverb a; a.prepare (sr); a.setParams (0.9f, 250.0f); Reverb b; b.prepare (sr); b.setParams (0.9f, 250.0f); b.setExtras (12.0f, 0.5f, 0.8f);
        double dev = 0; for (int n = 0; n < 24000; ++n) { float la, ra, lb, rb; float in = n == 0 ? 1.0f : 0.0f; a.process (in, la, ra); b.process (in, lb, rb); dev = std::max (dev, (double) std::fabs (la - lb) + std::fabs (ra - rb)); }
        std::snprintf (d, sizeof d, "max difference %.2e", dev); check ("reverb default extras unchanged", dev < 1e-7, d);
        auto firstSound = [&] (float preMs) { Reverb r; r.prepare (sr); r.setParams (0.9f, 250.0f); r.setExtras (preMs, 0.5f, 0.8f); for (int n = 0; n < 24000; ++n) { float l, rr; r.process (n == 0 ? 1.0f : 0.0f, l, rr); if (std::fabs (l) + std::fabs (rr) > 1e-4f) return n; } return -1; };
        int f12 = firstSound (12), f80 = firstSound (80);
        std::snprintf (d, sizeof d, "first sound at %d and %d samples", f12, f80); check ("reverb pre-delay delays the tail", f80 > f12 + 2500, d);
    }
    { // sequencer: free clock timing, slides, accents, gate
        SeqData data; Sequencer sq; sq.prepare (sr);
        for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (i % 12, i != 3, i == 4, i == 6));
        SeqSettings c; c.mode = 1; c.rate = 2; c.syncHost = false; c.bpm = 120.0f; c.gate = 0.5f; c.steps = 16; c.startHost = c.startKey = c.startNote = false; sq.setSettings (c);
        std::vector<SeqEvent> ev; HostTime h; const int block = 256, total = 95000;
        for (int pos = 0; pos < total; pos += block) { sq.run (data, h, pos, block); for (int i = 0; i < sq.numEvents(); ++i) ev.push_back (sq.event (i)); }
        int ons = 0, offs = 0; std::vector<int> onAt; bool accOk = true, slideOk = false;
        for (auto& e : ev) { if (e.type == SeqEvent::On) { ++ons; onAt.push_back (e.at); if (ons == 4 && ! e.accent) accOk = false; if (ons == 6 && e.slide) slideOk = true; } else ++offs; }
        // 120 bpm, 1/16 = 0.125 s = 6000 samples. In 2 s: steps at 0, 6000, ... (16 steps, 1 rest) -> 16 on-times, 15 notes (step 3 is a rest)
        std::snprintf (d, sizeof d, "%d note ons, %d offs, step 1 at %d, step 2 at %d", ons, offs, onAt.size() > 1 ? onAt[1] : -1, onAt.size() > 2 ? onAt[2] : -1);
        check ("seq free clock: 120 bpm 1/16", onAt.size() > 2 && onAt[0] == 0 && onAt[1] == 6000 && onAt[2] == 12000, d);
        check ("seq: rest skipped, accent and slide flags", ons == 15 && accOk && slideOk, d);
        // the note before a slide step must not be released
        bool noOffBeforeSlide = true; int on6 = -1; for (size_t i = 0; i < ev.size(); ++i) if (ev[i].type == SeqEvent::On && ev[i].slide) { on6 = ev[i].at; }
        for (auto& e : ev) if (e.type == SeqEvent::Off && e.at > 5 * 6000 && e.at < on6) noOffBeforeSlide = false;
        check ("seq: slide keeps previous gate open", on6 == 6 * 6000 && noOffBeforeSlide, "no note off between step 5 and the slide step");
    }
    { // sequencer: ratchets and chance
        auto runSeq = [&] (SeqData& data, int total, std::vector<SeqEvent>& ev, float gate)
        {
            Sequencer sq; sq.prepare (sr); SeqSettings c; c.mode = 1; c.rate = 2; c.syncHost = false; c.bpm = 120.0f; c.gate = gate; c.steps = 16; c.startHost = c.startKey = c.startNote = false; sq.setSettings (c);
            HostTime h; for (int pos = 0; pos < total; pos += 256) { sq.run (data, h, pos, 256); for (int i = 0; i < sq.numEvents(); ++i) ev.push_back (sq.event (i)); }
        };
        { // ratchet: step 0 is x3, step 1 is x2, everything else plain
            SeqData data; for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (i, true, false, false, 0, i == 0 ? 2 : (i == 1 ? 1 : 0)));
            std::vector<SeqEvent> ev; runSeq (data, 6000 * 4, ev, 0.5f);
            std::vector<int> onAt; for (auto& e : ev) if (e.type == SeqEvent::On) onAt.push_back (e.at);
            std::snprintf (d, sizeof d, "on times %d %d %d | %d %d | %d", onAt.size() > 0 ? onAt[0] : -1, onAt.size() > 1 ? onAt[1] : -1, onAt.size() > 2 ? onAt[2] : -1, onAt.size() > 3 ? onAt[3] : -1, onAt.size() > 4 ? onAt[4] : -1, onAt.size() > 5 ? onAt[5] : -1);
            check ("seq ratchet x3 then x2 split the step evenly", onAt.size() >= 6 && onAt[0] == 0 && onAt[1] == 2000 && onAt[2] == 4000 && onAt[3] == 6000 && onAt[4] == 9000 && onAt[5] == 12000, d);
            int on = 0, off = 0; for (auto& e : ev) (e.type == SeqEvent::On ? on : off)++;
            check ("seq ratchet notes all get released", off >= on - 1, "offs follow ons");
        }
        { // chance 50 percent: about half of 1600 steps play; chance 25: about a quarter; 100 plays all
            auto count = [&] (int chance, int& played)
            {
                SeqData data; for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (0, true, false, false, chance, 0));
                std::vector<SeqEvent> ev; runSeq (data, 6000 * 1600, ev, 0.5f); played = 0; for (auto& e : ev) if (e.type == SeqEvent::On) ++played;
            };
            int a = 0, b = 0, c2 = 0; count (0, a); count (2, b); count (3, c2);
            std::snprintf (d, sizeof d, "100%%: %d, 50%%: %d, 25%%: %d of 1600", a, b, c2);
            check ("seq chance 100/50/25 percent", a >= 1600 && a <= 1601 && b > 700 && b < 900 && c2 > 330 && c2 < 470, d);
        }
        { // a slide step that chance drops must not leave the previous note stuck open
            SeqData data; for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (0, true, false, i % 2 == 1, i % 2 == 1 ? 3 : 0, 0));
            std::vector<SeqEvent> ev; runSeq (data, 6000 * 16 * 60, ev, 0.5f);
            int open = 0, maxOpen = 0, nOn = 0; for (auto& e : ev) { if (e.type == SeqEvent::On) { ++nOn; open = 1; } else open = 0; (void) maxOpen; }
            // every On must be followed by an Off before the next non-slide On: count consecutive Ons without an Off
            int run = 0, worst = 0; for (auto& e : ev) { if (e.type == SeqEvent::On && ! e.slide) { run = 0; } if (e.type == SeqEvent::Off) run = 0; else ++run; worst = std::max (worst, run); }
            std::snprintf (d, sizeof d, "%d ons, longest On run without an Off %d", nOn, worst);
            check ("seq dropped slide step does not stick a note", nOn > 100 && worst <= 2, d);
        }
    }
    { // sequencer: host sync follows ppq and starts on the grid
        SeqData data; Sequencer sq; sq.prepare (sr); for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (i, true, false, false));
        SeqSettings c; c.mode = 1; c.rate = 2; c.syncHost = true; c.steps = 16; c.startHost = true; c.startKey = false; c.startNote = false; sq.setSettings (c);
        HostTime h; h.valid = true; h.playing = false; h.bpm = 120.0; h.ppq = 0.0; sq.run (data, h, 0, 256);
        check ("seq waits while host is stopped", sq.numEvents() == 0 && ! sq.isRunning(), "no events");
        h.playing = true; h.ppq = 1.1; sq.run (data, h, 0, 6000);   // starts mid beat: next 1/16 boundary is at ppq 1.25 -> 0.15 beat = 0.075 s = 3600 samples
        std::snprintf (d, sizeof d, "first on at %d (want 3600), step index %d", sq.numEvents() ? sq.event (0).at : -1, sq.currentStep());
        check ("seq host start lands on the grid", sq.numEvents() > 0 && std::abs (sq.event (0).at - 3600) <= 1 && sq.event (0).note == kSeqBaseNote + 5, d);
        h.playing = false; sq.run (data, h, 0, 256); bool off = false; for (int i = 0; i < sq.numEvents(); ++i) off = off || sq.event (i).type == SeqEvent::Off;
        check ("seq stops and releases with the host", ! sq.isRunning() && off, "note off sent");
    }
    { // key start, trigger note, arp
        SeqData data; Sequencer sq; sq.prepare (sr); for (int i = 0; i < 16; ++i) data.set (0, i, SeqData::pack (i, true, false, false));
        SeqSettings c; c.mode = 1; c.syncHost = false; c.bpm = 120; c.startHost = false; c.startKey = true; c.startNote = true; c.triggerNote = 24; sq.setSettings (c);
        HostTime h; sq.run (data, h, 0, 256); check ("seq idle until a key", ! sq.isRunning(), "");
        sq.keyOn (48); sq.run (data, h, 0, 256);
        check ("seq starts on key press, key transposes", sq.isRunning() && sq.numEvents() > 0 && sq.event (0).note == kSeqBaseNote + 0 + (48 - 36), "first note = C2 + 12");
        sq.keyOff (48); sq.run (data, h, 0, 256); check ("seq stops on key release", ! sq.isRunning(), "");
        sq.keyOn (24); sq.keyOff (24); sq.run (data, h, 0, 256); check ("trigger note toggles on", sq.isRunning() && sq.consumesKey (24), "");
        sq.keyOn (24); sq.run (data, h, 0, 256); check ("trigger note toggles off", ! sq.isRunning(), "");
        // arp up over held C3 E3 G3
        SeqSettings a; a.mode = 2; a.syncHost = false; a.bpm = 120; a.rate = 2; a.arpPattern = 0; a.arpOctaves = 2; a.gate = 0.5f; sq = Sequencer(); sq.prepare (sr); sq.setSettings (a);
        sq.keyOn (60); sq.keyOn (64); sq.keyOn (67); std::vector<int> notes; for (int pos = 0; pos < 48000; pos += 256) { sq.run (data, h, pos, 256); for (int i = 0; i < sq.numEvents(); ++i) if (sq.event (i).type == SeqEvent::On) notes.push_back (sq.event (i).note); }
        bool up = notes.size() >= 8 && notes[0] == 60 && notes[1] == 64 && notes[2] == 67 && notes[3] == 72 && notes[4] == 76 && notes[5] == 79 && notes[6] == 60;
        std::snprintf (d, sizeof d, "%d notes, first %d %d %d %d", (int) notes.size(), notes.size() > 0 ? notes[0] : -1, notes.size() > 1 ? notes[1] : -1, notes.size() > 2 ? notes[2] : -1, notes.size() > 3 ? notes[3] : -1);
        check ("arp up over 2 octaves", up, d);
        a.arpLatch = true; sq = Sequencer(); sq.prepare (sr); sq.setSettings (a); sq.keyOn (60); sq.keyOff (60); sq.run (data, h, 0, 256);
        check ("arp latch keeps playing after release", sq.isRunning(), "");
    }
    { // engine: sequencer notes are finite; squelch raises the resonance bump; 8x works; default Params unchanged vs plain notes
        auto render = [&] (bool squelch, int os, bool accent, std::vector<float>& out)
        {
            Params p; p.cutoffHz = 700; p.reso = 0.55f; p.fenvAmt = 0.4f; p.squelch = squelch; p.squelchAmt = 0.8f; p.accAmt = 0.6f;
            Engine e; e.setOversampling (os); e.prepare (sr); e.setParams (p);
            out.assign (48000, 0.0f);
            e.seqNoteOn (38, false, false, 0.08f); for (int i = 0; i < 6000; i += 64) e.process (out.data() + i, 64);
            e.seqNoteOff (); for (int i = 6000; i < 9000; i += 64) e.process (out.data() + i, 64);
            e.seqNoteOn (38, accent, false, 0.08f); for (int i = 9000; i < 15000; i += 64) e.process (out.data() + i, 64);
            e.seqNoteOn (45, false, true, 0.08f); for (int i = 15000; i < 21000; i += 64) e.process (out.data() + i, std::min (64, 21000 - i));
            e.seqNoteOff (); for (int i = 21000; i < 24000; i += 64) e.process (out.data() + i, 64);
        };
        std::vector<float> a, b, c8; render (false, 4, true, a); render (true, 4, true, b); render (true, 8, true, c8);
        bool fin = true; for (auto v : a) fin = fin && std::isfinite (v); for (auto v : b) fin = fin && std::isfinite (v); for (auto v : c8) fin = fin && std::isfinite (v);
        auto pkOf = [&] (const std::vector<float>& v, int f, int t) { float m = 0; for (int i = f; i < t; ++i) m = std::max (m, std::fabs (v[(size_t) i])); return m; };
        double ra = 0, rb = 0, rd = 0; for (int i = 9000; i < 15000; ++i) { ra += a[(size_t) i] * a[(size_t) i]; rb += b[(size_t) i] * b[(size_t) i]; rd += (a[(size_t) i] - b[(size_t) i]) * (a[(size_t) i] - b[(size_t) i]); }
        std::snprintf (d, sizeof d, "accent step: rms squelch off %.3f, on %.3f, difference to off %.1f dB", std::sqrt (ra / 6000), std::sqrt (rb / 6000), 10 * std::log10 (rd / (ra + 1e-9) + 1e-12));
        check ("squelch changes the accent step", fin && rd / (ra + 1e-9) > 0.1, d);
        std::snprintf (d, sizeof d, "peak %.2f", pkOf (c8, 0, 24000)); check ("8x oversampling finite and audible", fin && pkOf (c8, 0, 24000) > 0.05f && pkOf (c8, 0, 24000) < 3.0f, d);
        // no squelch and no accent: sequencer voice behaves like a normal voice (same level as a key note)
        Params p; p.cutoffHz = 900; Engine k; k.prepare (sr); k.setParams (p); Engine s; s.prepare (sr); s.setParams (p);
        std::vector<float> ka (12100), sa (12100); k.noteOn (40, 1.0f); s.seqNoteOn (40, false, false, 0.08f);
        for (int i = 0; i < 12000; i += 64) { k.process (ka.data() + i, 64); s.process (sa.data() + i, 64); }
        double dev = 0; for (int i = 0; i < 12000; ++i) dev = std::max (dev, (double) std::fabs (ka[(size_t) i] - sa[(size_t) i]));
        std::snprintf (d, sizeof d, "max difference %.2e", dev); check ("sequencer voice equals key voice", dev < 1e-5, d);
    }
    printf (fails ? "\n%d check(s) failed\n" : "\nall checks passed\n", fails);
    return fails ? 1 : 0;
}
