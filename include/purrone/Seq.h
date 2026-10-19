// PurrOne step sequencer and arpeggiator clock. Header only, no JUCE types, no allocation, no locks.
// It does not make sound. It turns time (host position or its own clock) and held keys into note events with sample offsets.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace purrone {

constexpr int kSeqMaxSteps = 64;
constexpr int kSeqPatterns = 4;
constexpr int kSeqBaseNote = 36;      // step value 0 = C2

// One step packed in 9 bits: note 0..36 (6 bits), gate, accent, slide. Atomic so the editor can edit while audio runs.
struct SeqData
{
    std::array<std::array<std::atomic<uint16_t>, kSeqMaxSteps>, kSeqPatterns> step;
    SeqData() { clear(); }
    SeqData (const SeqData&) = delete;
    static uint16_t pack (int note, bool gate, bool acc, bool slide) { return (uint16_t) ((std::min (std::max (note, 0), 36) & 63) | (gate ? 64 : 0) | (acc ? 128 : 0) | (slide ? 256 : 0)); }
    void clear() { for (auto& p : step) for (auto& s : p) s.store (pack (0, false, false, false), std::memory_order_relaxed); }
    uint16_t get (int pat, int i) const { return step[(size_t) (pat & 3)][(size_t) (i & 63)].load (std::memory_order_relaxed); }
    void set (int pat, int i, uint16_t v) { step[(size_t) (pat & 3)][(size_t) (i & 63)].store (v, std::memory_order_relaxed); }
    static int note (uint16_t v) { return v & 63; }
    static bool gate (uint16_t v) { return (v & 64) != 0; }
    static bool accent (uint16_t v) { return (v & 128) != 0; }
    static bool slide (uint16_t v) { return (v & 256) != 0; }
};

struct SeqSettings
{
    int mode = 0;               // 0 off, 1 sequence, 2 arp
    int rate = 2;               // 0 1/8, 1 1/8T, 2 1/16, 3 1/16T, 4 1/32
    bool syncHost = true;       // follow the host tempo and position
    float bpm = 138.0f;         // used when not following the host
    float swing = 0.5f;         // 0.5 straight .. 0.75
    float gate = 0.6f;          // note length as share of the step
    float human = 0.0f;
    int transpose = 0;
    int steps = 16;
    int pattern = 0;
    int arpPattern = 0;         // 0 up, 1 down, 2 up/down, 3 random, 4 played order
    int arpOctaves = 1;
    bool arpLatch = false;
    bool startHost = true, startKey = true, startNote = false;
    int triggerNote = 24;
    bool keysTranspose = true;
};

struct HostTime { bool valid = false, playing = false; double ppq = 0.0, bpm = 0.0; };

struct SeqEvent { enum Type { On, Off } type = Off; int at = 0; int note = 0; bool accent = false, slide = false; };

class Sequencer
{
public:
    static constexpr int kMaxEvents = 96;

    void prepare (double s) { sr = s; reset(); }
    void reset()
    {
        running = false; noteActive = false; activeNote = 0; beat = 0.0; nextK = 0; offBeat = 1.0e18; nOut = 0; heldN = 0; latchN = 0; physN = 0; arpIdx = 0; arpDir = 1;
        trigState = false; keysDown = 0; rngState = 12345u;
    }

    // ---- input events, call in time order ----
    void keyOn (int note)
    {
        if (note == cfg.triggerNote && cfg.startNote && cfg.mode != 0) { trigState = ! trigState; return; }
        ++keysDown;
        if (physN < 16) phys[physN++] = note;
        if (cfg.arpLatch && keysDown == 1) latchN = 0;                 // first key after a release starts a fresh latched chord
        addTo (held, heldN, note);
        if (cfg.arpLatch) addTo (latched, latchN, note);
    }
    void keyOff (int note)
    {
        if (note == cfg.triggerNote && cfg.startNote && cfg.mode != 0) return;
        keysDown = std::max (0, keysDown - 1);
        removeFrom (phys, physN, note);
        removeFrom (held, heldN, note);
    }
    void allKeysOff() { heldN = 0; physN = 0; keysDown = 0; if (! cfg.arpLatch) latchN = 0; }
    // true when a note-on or note-off should not reach the synth (the sequencer owns the keys)
    bool consumesKey (int note) const
    {
        if (cfg.mode == 0) return false;
        if (cfg.startNote && note == cfg.triggerNote) return true;
        return cfg.mode == 2 || cfg.startKey;
    }

    void setSettings (const SeqSettings& c) { cfg = c; cfg.steps = std::min (std::max (cfg.steps, 1), kSeqMaxSteps); }
    const SeqSettings& settings() const { return cfg; }
    bool isRunning() const { return running; }
    int currentStep() const { return running ? lastStep : -1; }       // for the editor playhead
    bool triggerState() const { return trigState; }
    void setManual (bool on) { manual = on; }                       // the Play button on the Seq page

    // Advance by count samples starting at sample offset 'start' of the block. Events are appended with 'at' relative to the block.
    // host describes the host position at the start of THIS segment (the caller advances it).
    void run (const SeqData& data, const HostTime& host, int start, int count)
    {
        nOut = 0;
        const bool want = shouldRun (host);
        const bool useHost = cfg.syncHost && host.valid && host.playing;
        double bpmNow = (cfg.syncHost && host.valid && host.bpm > 1.0) ? host.bpm : (double) cfg.bpm;
        const double bps = bpmNow / 60.0 / sr;
        const double stepBeats = stepLength();

        if (! want)
        {
            if (running) stopNote (start);
            running = false; return;
        }
        if (! running)
        {
            running = true; startedOnHost = useHost;
            if (useHost) { beat = host.ppq; nextK = (long long) std::ceil (beat / stepBeats - 1.0e-9); }
            else { beat = 0.0; nextK = 0; }
            nextT = stepTime (nextK, stepBeats); lastStep = -1;
            arpIdx = 0; arpDir = 1;
        }
        else if (useHost)
        {
            if (! startedOnHost || std::fabs (host.ppq - beat) > 0.02) { beat = host.ppq; nextK = (long long) std::ceil (beat / stepBeats - 1.0e-9); nextT = stepTime (nextK, stepBeats); }
            startedOnHost = true;
        }
        else if (startedOnHost) { startedOnHost = false; }     // host stopped: keep going on the free clock from here

        const double beat1 = beat + count * bps;
        for (;;)
        {
            const bool stepDue = nextT < beat1, offDue = noteActive && offBeat < beat1;
            if (! stepDue && ! offDue) break;
            if (offDue && (! stepDue || offBeat <= nextT))
            {
                const int at = sampleAt (offBeat, bps, start, count);
                // a slide into the next step keeps this note's gate open
                if (! slideNext (data)) emitOff (at);    // otherwise the gate stays open and the next step slides on from this note
                offBeat = 1.0e18;
                continue;
            }
            fireStep (data, sampleAt (nextT, bps, start, count), stepBeats);
            ++nextK; nextT = stepTime (nextK, stepBeats);
        }
        beat = beat1;
    }

    int numEvents() const { return nOut; }
    const SeqEvent& event (int i) const { return out[(size_t) i]; }

private:
    static void addTo (int* a, int& n, int v) { for (int i = 0; i < n; ++i) if (a[i] == v) return; if (n < 16) a[n++] = v; }
    static void removeFrom (int* a, int& n, int v) { for (int i = 0; i < n; ++i) if (a[i] == v) { for (int j = i; j < n - 1; ++j) a[j] = a[j + 1]; --n; return; } }

    bool shouldRun (const HostTime& host) const
    {
        if (cfg.mode == 0) return false;
        if (cfg.mode == 2) return cfg.arpLatch ? (latchN > 0) : (heldN > 0);
        if (manual) return true;
        const bool any = cfg.startHost || cfg.startKey || cfg.startNote;
        if (! any) return true;                                                     // nothing selected: free running
        if (cfg.startHost && cfg.syncHost && host.valid && host.playing) return true;
        if (cfg.startKey && physN > 0) return true;
        if (cfg.startNote && trigState) return true;
        return false;
    }
    double stepLength() const
    {
        static const double beats[5] = { 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
        return beats[std::min (std::max (cfg.rate, 0), 4)];
    }
    double stepTime (long long k, double sb)
    {
        const double sw = (k & 1) ? (2.0 * std::min (std::max ((double) cfg.swing, 0.5), 0.75) - 1.0) : 0.0;
        double t = ((double) k + sw) * sb;
        if (cfg.human > 0.001f) t += rand01() * cfg.human * 0.12 * sb;
        return t;
    }
    int sampleAt (double b, double bps, int start, int count) const
    {
        const int off = (int) std::floor ((b - beat) / bps + 0.5);
        return start + std::min (std::max (off, 0), std::max (count - 1, 0));
    }
    float rand01() { rngState = rngState * 1664525u + 1013904223u; return (float) (rngState >> 8) * (1.0f / 16777216.0f); }

    void push (const SeqEvent& e) { if (nOut < kMaxEvents) out[(size_t) nOut++] = e; }
    void emitOff (int at) { if (noteActive) { SeqEvent e; e.type = SeqEvent::Off; e.at = at; e.note = activeNote; push (e); noteActive = false; } }
    void stopNote (int at) { emitOff (at); offBeat = 1.0e18; }
    bool slideNext (const SeqData& d) const
    {
        if (cfg.mode != 1) return false;
        const int j = (lastStep + 1) % cfg.steps;
        const uint16_t v = d.get (cfg.pattern, j);
        return SeqData::gate (v) && SeqData::slide (v);
    }

    int keyOffset() const
    {
        if (! cfg.keysTranspose || ! cfg.startKey || physN == 0) return 0;
        int lo = phys[0]; for (int i = 1; i < physN; ++i) lo = std::min (lo, phys[i]);
        return lo - kSeqBaseNote;
    }

    void fireStep (const SeqData& d, int at, double stepBeats)
    {
        const int idx = (int) (nextK % cfg.steps);
        lastStep = idx;
        int note = -1; bool acc = false, sl = false;
        if (cfg.mode == 1)
        {
            const uint16_t v = d.get (cfg.pattern, idx);
            if (SeqData::gate (v)) { note = kSeqBaseNote + SeqData::note (v) + cfg.transpose + keyOffset(); acc = SeqData::accent (v); sl = SeqData::slide (v); }
        }
        else
        {
            note = arpNote();
            if (note >= 0) note += cfg.transpose;
        }
        if (note < 0) { return; }                                                   // rest: any earlier note ends at its own gate time
        note = std::min (std::max (note, 0), 127);
        if (noteActive && ! sl) emitOff (at);                                        // not a slide: end the previous note first
        SeqEvent e; e.type = SeqEvent::On; e.at = at; e.note = note; e.accent = acc; e.slide = sl; push (e);
        noteActive = true; activeNote = note;
        offBeat = nextT + std::min (std::max ((double) cfg.gate, 0.05), 1.0) * stepBeats;
    }

    int arpNote()
    {
        int list[16 * 4]; int n = 0;
        const int* src = cfg.arpLatch ? latched : held; const int cnt = cfg.arpLatch ? latchN : heldN;
        if (cnt == 0) return -1;
        int base[16]; for (int i = 0; i < cnt; ++i) base[i] = src[i];
        if (cfg.arpPattern != 4) std::sort (base, base + cnt);
        const int octs = std::min (std::max (cfg.arpOctaves, 1), 4);
        for (int o = 0; o < octs; ++o) for (int i = 0; i < cnt; ++i) list[n++] = base[i] + 12 * o;
        int pick;
        switch (cfg.arpPattern)
        {
            case 1: pick = n - 1 - (arpIdx % n); ++arpIdx; break;
            case 2:
            {
                const int cycle = n > 1 ? 2 * n - 2 : 1, pos = arpIdx % cycle; ++arpIdx;
                pick = pos < n ? pos : cycle - pos; break;
            }
            case 3: pick = (int) (rand01() * (float) n) % n; break;
            default: pick = arpIdx % n; ++arpIdx; break;
        }
        return list[std::min (std::max (pick, 0), n - 1)];
    }

    double sr = 48000.0; SeqSettings cfg;
    bool manual = false, running = false, startedOnHost = false, noteActive = false, trigState = false;
    double beat = 0.0, nextT = 0.0, offBeat = 1.0e18; long long nextK = 0;
    int activeNote = 0, lastStep = -1, arpIdx = 0, arpDir = 1, keysDown = 0;  // arpDir kept for compatibility
    int held[16] {}, latched[16] {}, phys[16] {}; int heldN = 0, latchN = 0, physN = 0;
    std::array<SeqEvent, kMaxEvents> out {}; int nOut = 0; uint32_t rngState = 12345u;
};

} // namespace purrone
