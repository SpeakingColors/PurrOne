// PurrOne mono synth engine. Header only, no JUCE types, no allocation after construction.
// Original design inspired by classic two oscillator mono synths. No vendor code, samples or presets.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace purrone {

constexpr float kPi = 3.14159265358979323846f;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

// ---- band-limited step residuals (2 point polyBLEP) ----
inline float blepAfter (float x)  { return x + x - x * x - 1.0f; }      // x in [0,1): time since step in samples
inline float blepBefore (float x) { return x * x + x + x + 1.0f; }      // x in [-1,0): time to step

inline float polyBlep (float t, float dt)
{
    if (t < dt)       return blepAfter (t / dt);
    if (t > 1.0f - dt) return blepBefore ((t - 1.0f) / dt);
    return 0.0f;
}

struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0.0f, a = 0.005f, d = 0.2f, s = 0.7f, r = 0.2f; // seconds / level
    float sr = 48000.0f;

    static float timeFromKnob (float k) { return 0.002f * std::pow (3000.0f, clampf (k, 0.0f, 1.0f)); } // 2 ms .. 6 s

    void gateOn()  { stage = Attack; }                 // retrigger from the current level
    void gateOff() { if (stage != Idle) stage = Release; }
    void reset()   { stage = Idle; value = 0.0f; }

    float tick()
    {
        auto coef = [this] (float t) { return 1.0f - std::exp (-1.0f / (std::max (t, 0.001f) * sr)); };
        switch (stage)
        {
            case Idle: break;
            case Attack:
                value += (1.25f - value) * coef (a * 0.45f);   // RC style charge toward 1.25, ends at 1
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value += (s - value) * coef (d * 0.45f);
                if (std::abs (value - s) < 1.0e-4f) { value = s; stage = Sustain; }
                break;
            case Sustain: value = s; break;
            case Release:
                value += (0.0f - value) * coef (r * 0.45f);
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
        }
        return value;
    }
    bool active() const { return stage != Idle; }
};

struct Params
{
    // Oscillator A: keyboard controlled. octave 0..3 = 16' 8' 4' 2'; tune in semitones (-6..+6)
    int   aOct = 1;   float aTune = 0.0f; int aSemi = 0;  bool aSaw = true, aPulse = false; float aPW = 0.5f; bool aSync = false;
    // Oscillator B
    int   bOct = 1;   float bTune = 0.0f; int bSemi = 0;  bool bSaw = true, bTri = false, bPulse = false; float bPW = 0.5f;
    bool  bLow = false, bKbd = true;
    // Mixer 0..1
    float mixA = 0.8f, mixB = 0.0f, mixNoise = 0.0f, mixSub = 0.0f;
    // Filter
    float cutoffHz = 3000.0f, reso = 0.1f, fenvAmt = 0.0f, kbdAmt = 0.5f; // fenvAmt -1..1
    int   filterMode = 0;   // 0 low pass, 1 high pass, 2 band pass (same 4 pole ladder, stage outputs combined)
    int   colourPos = 2;    // where Drive/Transistor/Fuzz sit: 0 before the filter, 1 after it, 2 both (light before, full after)
    // Envelopes: knob 0..1 for times, sustain 0..1
    float fA = 0.0f, fD = 0.4f, fS = 0.3f, fR = 0.3f;
    float aA = 0.0f, aD = 0.4f, aS = 0.8f, aR = 0.3f;
    // Glide
    float glide = 0.0f; bool glideAuto = false;
    // LFO
    float lfoHz = 5.0f; bool lfoSaw = false, lfoTri = true, lfoPulse = false;
    // Mod matrix. route/dest: 0 off, 1 direct, 2 wheel. sources: 0 filter env, 1 osc B, 2 LFO
    float modAmt[3] = { 0, 0, 0 };  int modRoute[3] = { 0, 0, 0 };
    // destinations: 0 A freq, 1 A PW, 2 B freq, 3 B PW, 4 filter
    int   modDest[5] = { 0, 0, 0, 0, 0 };
    float modWheel = 0.0f;
    // Performance: wheel and aftertouch combined by the host code. Cutoff in octaves, vibrato depth in semitones (uses the LFO rate)
    float perfCutOct = 0.0f, perfVibSemi = 0.0f;
    // Unison: 1 to 5 stacked copies of both oscillators (A copies are off while hard sync is on), detuned up to 100 cents, one shared filter
    int uniVoices = 1; float uniDetune = 0.3f;
    // Keyboard
    bool retrig = false, repeat = false, drone = false;
    // Output
    float drive = 0.0f;   // 0..1 analog clip (symmetric soft clip)
    float xstr = 0.0f;    // 0..1 Neve-style transistor stage (asymmetric, even harmonics, low-end weight)
    float fuzz = 0.0f;    // 0..1 asymmetric fuzz ahead of the filter
    float volume = 0.7f;
    // Sequencer feel: accent amount (louder and brighter on accented steps) and the optional 303 squelch
    // (accents and slides bump the resonance and the filter envelope). Squelch is off by default.
    bool  squelch = false; float squelchAmt = 0.6f, accAmt = 0.4f;
    // Random (sample and hold) modulation; depth 0 = off
    float rndHz = 4.0f, rndSlew = 0.35f, rndDepth = 0.0f; bool rndCut = true, rndPA = false, rndPB = false, rndPW = true;
    // Tuning: A4 offset in semitones and per pitch class offsets in semitones (C = 0). All zero = equal temperament at 440 Hz.
    float tuneRef = 0.0f; float tuneOff[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};

class Engine
{
public:
    // factor 4 (normal) or 8 (high): the voice runs at factor x the host rate
    void setOversampling (int factor) { const int f = factor >= 8 ? 8 : 4; if (f != os) { os = f; prepare ((double) sr); } }
    int oversampling() const { return os; }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate; osr = sr * (float) os; xfC = 1.0f - std::exp (-6.2831853f * 90.0f / osr);
        for (auto* e : { &fenv, &aenv }) e->sr = osr;
        buildHalfband();
        reset();
    }

    void reset()
    {
        smoothInit = false; bend = 0.0f; xfLp = 0.0f; nLp = 0.0f; lastDrive = lastXs = lastFuzz = -1.0f;
        for (int k = 0; k < 4; ++k) { phAx[k] = std::fmod (0.13f + 0.618034f * (float) (k + 1), 1.0f); phBx[k] = std::fmod (0.41f + 0.381966f * (float) (k + 1), 1.0f); }
        for (int k = 0; k < 4; ++k) uDrift[k] = uDriftT[k] = 0.0f; uDriftCnt = 0;
        phA = phB = phSub = 0.0f; subFlip = 0.0f; fenv.reset(); aenv.reset();
        for (auto& s : st) s = 0.0f;
        accEnv = sqBump = accLvl = accTarget = 0.0f; glideOverride = 0.0f; rndPh = 0.0f; rndVal = rndTarget = 0.0f; h0Pos = 0; h0Buf.fill (0.0f);
        lfoPh = 0.0f; hbPos = 0; hbBuf.fill (0.0f); hb1Pos = 0; hb1Buf.fill (0.0f); heldCount = 0; curNote = -1; glidePitch = 60.0f;
        prevA = 0.0f; gateOn = false; repeatPh = 0.0f; seed = 22222u; dcX = dcY = 0.0f;
    }

    void setParams (const Params& p)
    {
        P = p;
        if (P.drive != lastDrive || P.xstr != lastXs || P.fuzz != lastFuzz || P.colourPos != lastPos)
        {
            lastDrive = P.drive; lastXs = P.xstr; lastFuzz = P.fuzz; lastPos = P.colourPos;
            preAmt = P.colourPos == 0 ? 1.0f : (P.colourPos == 1 ? 0.0f : kPreAmt);
            postAmt = P.colourPos == 0 ? 0.0f : 1.0f;
            mkT = shaperMakeup (P.drive * preAmt, P.xstr * preAmt, P.fuzz * preAmt, 0.35f);
            mkPT = shaperMakeup (P.drive * postAmt, P.xstr * postAmt, P.fuzz * postAmt, 0.9f);
            if (! smoothInit) { mkQ = mkT; mkPQ = mkPT; }
        }
        if (! smoothInit) { Q = p; smoothInit = true; }
    }
    void setPitchBend (float semitones) { bend = semitones; }

    void noteOn (int note, float /*velocity*/)
    {
        bool wasHeld = heldCount > 0;
        if (heldCount < kMaxHeld) held[heldCount++] = note;
        else { held[kMaxHeld - 1] = note; }
        retargetNote (wasHeld);
    }

    void noteOff (int note)
    {
        for (int i = 0; i < heldCount; ++i)
            if (held[i] == note) { for (int j = i; j < heldCount - 1; ++j) held[j] = held[j + 1]; --heldCount; break; }
        if (heldCount == 0) { gateOn = false; if (! P.drone) { fenv.gateOff(); aenv.gateOff(); } }
        else retargetNote (true, false);
    }

    // ---- sequencer voice: one note at a time, never mixed with the key stack ----
    // slide: glide into the note from the previous one without retriggering the envelopes (only if the previous gate is still open)
    void seqNoteOn (int note, bool accent, bool slide, float slideSeconds)
    {
        const bool legato = slide && gateOn && curNote >= 0;
        heldCount = 1; held[0] = note;
        curNote = note;
        if (legato) glideOverride = std::max (0.005f, slideSeconds) * 0.35f;      // time constant: about 3 of them make the glide time
        else
        {
            glideOverride = 0.0f; glidePitch = (float) note;
            if (! P.drone) { fenv.gateOn(); aenv.gateOn(); }
        }
        gateOn = true;
        accTarget = accent ? 1.0f : 0.0f;
        if (accent) { accEnv = 1.0f; sqBump = std::min(1.5f, sqBump * 0.7f + 1.0f); }      // consecutive accents build up, like the 303 accent circuit
        if (legato) sqBump = std::max (sqBump, 0.6f);
    }
    void seqNoteOff()
    {
        heldCount = 0; gateOn = false; glideOverride = 0.0f;
        if (! P.drone) { fenv.gateOff(); aenv.gateOff(); }
    }

    void allNotesOff() { heldCount = 0; gateOn = false; fenv.gateOff(); aenv.gateOff(); }
    bool isActive() const { return aenv.active() || gateOn || P.drone; }

    // mono output, n samples
    void process (float* out, int n)
    {
        applyEnvParams();
        if (os == 8)
        {
            for (int i = 0; i < n; ++i)
            {
                float z[4];
                for (int k = 0; k < 4; ++k) { const float a = tick2x(), b = tick2x(); z[k] = half0 (a, b); }
                out[i] = decimate (half1 (z[0], z[1]), half1 (z[2], z[3]));
            }
            return;
        }
        for (int i = 0; i < n; ++i)
        {
            const float z0 = tick2x(), z1 = tick2x(), z2 = tick2x(), z3 = tick2x();
            out[i] = decimate (half1 (z0, z1), half1 (z2, z3));
        }
    }

    // for the editor's transfer curve: what the colour stages do to one input value (level matched like the real signal path)
    static float colourCurve (float x, float drive, float xs, float fuzz) { return shaper (x, drive, xs, fuzz) * shaperMakeup (drive, xs, fuzz, 0.9f); }

private:
    static constexpr int kMaxHeld = 16;
    Params P, Q;                 // P = target values, Q = smoothed values used by the voice
    int lastPos = -1; float preAmt = kPreAmt, postAmt = 1.0f;
    float lastDrive = -1.0f, lastXs = -1.0f, lastFuzz = -1.0f, mkT = 1.0f, mkQ = 1.0f, mkPT = 1.0f, mkPQ = 1.0f;
    float nLp = 0.0f; float xfLp = 0.0f, xfC = 0.012f;         // transformer low-end state, xfC set in prepare
    static constexpr float kPreAmt = 0.4f;   // pre-filter stage is a lighter version of the post-filter stage   // drive/fuzz loudness match

    // Three colour stages in series, each with its own knob:
    //  Drive: analog clip. Symmetric tanh soft clip, odd harmonics, the classic overdriven-opamp sound.
    //  Transistor: Neve-style stage. Asymmetric soft clip (the negative half rounds off earlier), so the
    //              added harmonics are mostly even and sound warm and thick, not buzzy.
    //  Fuzz: harder and lopsided on top of that.
    // Above about 80 percent each knob gets nastier (stacked stages, octave-up fuzz) using only smooth
    // curves, so it growls without turning into digital fizz.
    static float smoothstep01 (float t) { t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); return t * t * (3.0f - 2.0f * t); }
    static float asymTanh (float u, float nl) { return u >= 0.0f ? std::tanh (u) : nl * std::tanh (u / nl); }
    static float shaper (float x, float drive, float xs, float fuzz)
    {
        const float dn = smoothstep01 ((drive - 0.8f) * 5.0f), xn = smoothstep01 ((xs - 0.8f) * 5.0f), fn = smoothstep01 ((fuzz - 0.8f) * 5.0f);
        float y = x;
        if (drive > 0.001f)
        {
            y = std::tanh (x * (1.0f + drive * 5.0f + dn * 30.0f));
            if (dn > 0.0f) y += dn * (std::tanh (y * (2.0f + 10.0f * dn)) - y);          // second stage, driven by the first
        }
        if (xs > 0.001f)
        {
            const float nl = 1.0f - 0.3f * xs - 0.25f * xn;           // negative clip level
            const float u = y * (1.0f + xs * 5.0f + xn * 30.0f);
            y = asymTanh (u, nl);
            y += (0.3f * xs + 0.25f * xn) * y * y;                    // class A style even harmonic; the DC blocker removes the offset
            if (xn > 0.0f) { const float y2 = asymTanh (y * (2.0f + 10.0f * xn) + 0.15f * xn, nl); y += xn * (y2 - y); }
        }
        if (fuzz > 0.001f)
        {
            const float fz = 1.0f + fuzz * 14.0f + fn * 70.0f, bf = 0.5f * fuzz;
            y = std::tanh (y * fz + bf) - std::tanh (bf);
            if (fn > 0.0f)                                            // octave-up growl: smooth rectifier, no corner at zero
            {
                const float r = std::sqrt (y * y + 0.02f) - 0.1414f;
                y += fn * 0.7f * (r - 0.3f);
                y = std::tanh (y * (1.0f + 0.8f * fn));               // stage 3 tames what the rectifier added
            }
        }
        return y;
    }
    // Gain that keeps the level about the same as the clean path for a typical mixer signal,
    // so drive and fuzz add colour and grit instead of just changing the volume.
    static float shaperMakeup (float drive, float xs, float fuzz, float ref)
    {
        double a = 0.0, b = 0.0;
        for (int i = 0; i < 64; ++i)
        {
            const float x = ref * std::sin (6.2831853f * (float) i / 64.0f);
            const float y0 = shaper (x, 0.0f, 0.0f, 0.0f), y1 = shaper (x, drive, xs, fuzz);
            a += (double) y0 * y0; b += (double) y1 * y1;
        }
        const float m = b > 1.0e-9 ? (float) std::sqrt (a / b) : 1.0f;
        return m < 0.25f ? 0.25f : (m > 4.0f ? 4.0f : m);
    }
    bool smoothInit = false; float bend = 0.0f;
    float sr = 48000.0f, osr = 192000.0f;   // voice runs at 4x; two halfband stages bring it back down
    float phAx[4] {}, phBx[4] {};
    float uDrift[4] {}, uDriftT[4] {}; int uDriftCnt = 0;   // slow random wander of each unison copy, a few cents, like analog oscillators drifting
    float subFlip = 0.0f, phA = 0, phB = 0, phSub = 0, lfoPh = 0, prevA = 0, repeatPh = 0;
    float st[4] = { 0, 0, 0, 0 };
    Adsr fenv, aenv;
    int held[kMaxHeld] {}; int heldCount = 0, curNote = -1; bool gateOn = false;
    float glidePitch = 60.0f;
    int os = 4;
    float accEnv = 0.0f, sqBump = 0.0f, accLvl = 0.0f, accTarget = 0.0f, glideOverride = 0.0f;
    float rndPh = 0.0f, rndVal = 0.0f, rndTarget = 0.0f;
    uint32_t seed = 22222u;
    float dcX = 0, dcY = 0;
    // pending sync correction for the delayed osc A sample
    float prevPending = 0.0f;

    // halfband decimator
    static constexpr int kTaps = 95;
    std::array<float, kTaps> hb {};
    std::array<float, kTaps> hbBuf {};
    int hbPos = 0;

    // first decimation stage: 4x to 2x, short windowed sinc (cutoff 0.2 of the 4x rate)
    static constexpr int kTaps1 = 31;
    std::array<float, kTaps1> hb1 {}, hb1Buf {};
    int hb1Pos = 0;
    // 8x to 4x stage (same filter as half1, own history)
    std::array<float, kTaps1> h0Buf {}; int h0Pos = 0;
    float half0 (float a, float b)
    {
        h0Buf[(size_t) h0Pos] = a; h0Pos = (h0Pos + 1) % kTaps1;
        h0Buf[(size_t) h0Pos] = b; h0Pos = (h0Pos + 1) % kTaps1;
        float acc = 0.0f; int idx = h0Pos;
        for (int k = 0; k < kTaps1; ++k) { idx = (idx + kTaps1 - 1) % kTaps1; acc += hb1[(size_t) k] * h0Buf[(size_t) idx]; }
        return acc;
    }
    float half1 (float a, float b)
    {
        hb1Buf[(size_t) hb1Pos] = a; hb1Pos = (hb1Pos + 1) % kTaps1;
        hb1Buf[(size_t) hb1Pos] = b; hb1Pos = (hb1Pos + 1) % kTaps1;
        float acc = 0.0f; int idx = hb1Pos;
        for (int k = 0; k < kTaps1; ++k) { idx = (idx + kTaps1 - 1) % kTaps1; acc += hb1[(size_t) k] * hb1Buf[(size_t) idx]; }
        return acc;
    }
    void buildHalfband()
    {
        {
            double sum1 = 0.0; const int M1 = kTaps1 - 1;
            for (int i = 0; i < kTaps1; ++i)
            {
                const double x = i - M1 * 0.5, fcn = 0.2;
                const double s = (x == 0.0) ? 2.0 * fcn : std::sin (2.0 * kPi * fcn * x) / (kPi * x);
                const double w = 0.35875 - 0.48829 * std::cos (2 * kPi * i / M1) + 0.14128 * std::cos (4 * kPi * i / M1) - 0.01168 * std::cos (6 * kPi * i / M1);
                hb1[(size_t) i] = (float) (s * w); sum1 += s * w;
            }
            for (auto& c : hb1) c = (float) (c / sum1);
        }
        // windowed sinc lowpass at fs_out/2 (= osr/4), Blackman-Harris window, unity DC gain
        double sum = 0.0; const int M = kTaps - 1;
        for (int i = 0; i < kTaps; ++i)
        {
            double x = i - M * 0.5;
            const double fcn = 0.2125; // cutoff as fraction of the 2x rate (0.2125*96k = 20.4 kHz)
            double s = (x == 0.0) ? 2.0 * fcn : std::sin (2.0 * kPi * fcn * x) / (kPi * x);
            double w = 0.35875 - 0.48829 * std::cos (2 * kPi * i / M) + 0.14128 * std::cos (4 * kPi * i / M) - 0.01168 * std::cos (6 * kPi * i / M);
            hb[(size_t) i] = (float) (s * w); sum += s * w;
        }
        for (auto& c : hb) c = (float) (c / sum);
    }

    float decimate (float a, float b)
    {
        hbBuf[(size_t) hbPos] = a; hbPos = (hbPos + 1) % kTaps;
        hbBuf[(size_t) hbPos] = b; hbPos = (hbPos + 1) % kTaps;
        float acc = 0.0f; int idx = hbPos;
        for (int k = 0; k < kTaps; ++k) { idx = (idx + kTaps - 1) % kTaps; acc += hb[(size_t) k] * hbBuf[(size_t) idx]; }
        // gentle DC blocker at ~5 Hz
        float y = acc - dcX + 0.9993f * dcY; dcX = acc; dcY = y;
        return y;
    }

    void applyEnvParams()
    {
        fenv.a = Adsr::timeFromKnob (P.fA); fenv.d = Adsr::timeFromKnob (P.fD); fenv.s = P.fS; fenv.r = Adsr::timeFromKnob (P.fR);
        aenv.a = Adsr::timeFromKnob (P.aA); aenv.d = Adsr::timeFromKnob (P.aD); aenv.s = std::sqrt (std::max (P.aS, 0.0f));   // sustain taper: the middle of the knob is clearly between pluck and full aenv.r = Adsr::timeFromKnob (P.aR);
    }

    void retargetNote (bool legatoPlay, bool triggerAllowed = true)
    {
        // NORMAL = low note priority, single trigger. RETRIG = last note priority, retrigger.
        int target;
        if (P.retrig) target = held[heldCount - 1];
        else { target = held[0]; for (int i = 1; i < heldCount; ++i) target = std::min (target, held[i]); }
        bool newGate = ! legatoPlay || (P.retrig && triggerAllowed);
        if (! legatoPlay) glidePitch = (float) target;
        else if (P.glideAuto && curNote < 0) glidePitch = (float) target;
        curNote = target;
        if (newGate && triggerAllowed) { gateOn = true; if (! P.drone) { fenv.gateOn(); aenv.gateOn(); } }
        else gateOn = true;
    }

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (int32_t) seed * (1.0f / 2147483648.0f); }

    static float naiveSaw (float t) { return 2.0f * t - 1.0f; }
    static float naivePulse (float t, float pw) { return t < pw ? 1.0f : -1.0f; }

    float tick2x()
    {
        smoothParams();
        const float dtSec = 1.0f / osr;
        // ---- glide (exponential slew in semitones) ----
        float gt = Q.glide <= 0.001f ? 0.0f : 0.005f * std::pow (800.0f, Q.glide); // 5 ms .. 4 s
        if (glideOverride > 0.0f) gt = glideOverride;
        if (curNote >= 0)
        {
            if (gt <= 0.0f) glidePitch = (float) curNote;
            else glidePitch += ((float) curNote - glidePitch) * (1.0f - std::exp (-dtSec / gt));
        }
        // tuning: per pitch class offsets (interpolated while gliding) plus the A4 reference
        float tuneAdd = P.tuneRef;
        {
            const float gp = glidePitch + 1200.0f; const int k = (int) gp; const float fr = gp - (float) k;
            tuneAdd += P.tuneOff[k % 12] * (1.0f - fr) + P.tuneOff[(k + 1) % 12] * fr;
        }
        // accent and squelch envelopes
        accEnv -= accEnv * (1.0f - std::exp (-dtSec / 0.18f)); sqBump -= sqBump * (1.0f - std::exp (-dtSec / 0.14f));
        accLvl += (accTarget - accLvl) * (1.0f - std::exp (-dtSec / 0.004f));
        const float sq = Q.squelch ? Q.squelchAmt * sqBump : 0.0f;
        // random (sample and hold) modulation
        float rv = 0.0f;
        if (Q.rndDepth > 0.001f)
        {
            rndPh += Q.rndHz * dtSec; if (rndPh >= 1.0f) { rndPh -= 1.0f; rndTarget = rnd(); }
            const float tau = std::max (0.0f, Q.rndSlew) * 0.5f / std::max (0.1f, Q.rndHz);
            rndVal += (tau < 1.0e-5f ? 1.0f : (1.0f - std::exp (-dtSec / tau))) * (rndTarget - rndVal);
            rv = rndVal * Q.rndDepth;
        }
        const float note = glidePitch + bend + tuneAdd;

        // ---- envelopes and repeat ----
        if (Q.repeat && gateOn && ! Q.drone)
        {
            repeatPh += Q.lfoHz * dtSec;
            if (repeatPh >= 1.0f) { repeatPh -= 1.0f; fenv.gateOn(); aenv.gateOn(); }
            else if (repeatPh > 0.5f) { fenv.gateOff(); aenv.gateOff(); }
        }
        if (Q.drone && ! aenv.active()) { fenv.gateOn(); aenv.gateOn(); }
        float fe = fenv.tick(), ae = aenv.tick();

        // ---- LFO (mix of enabled shapes, scaled so a single shape peaks at +-1) ----
        lfoPh += Q.lfoHz * dtSec; if (lfoPh >= 1.0f) lfoPh -= 1.0f;
        float lfo = 0.0f; int ln = 0;
        if (Q.lfoSaw)   { lfo += naiveSaw (lfoPh); ++ln; }
        if (Q.lfoTri)   { lfo += 4.0f * std::abs (lfoPh - 0.5f) - 1.0f; ++ln; }
        if (Q.lfoPulse) { lfo += naivePulse (lfoPh, 0.5f); ++ln; }
        if (ln > 1) lfo /= (float) ln;

        // ---- oscillator B ----
        float freqB;
        if (Q.bLow) freqB = (Q.bKbd ? 0.25f * midiToHz (note + Q.bTune + (float) Q.bSemi) : 0.25f * 8.0f) * 0.0625f; // sub-audio
        else        freqB = midiToHz ((Q.bKbd ? note : 60.0f) + Q.bTune + (float) Q.bSemi + 12.0f * (Q.bOct - 1));
        float bFreqMod = 0.0f, bPwMod = 0.0f, aFreqMod = 0.0f, aPwMod = 0.0f, filMod = 0.0f;

        // modulation sources use the previous B sample (one tick delay is inaudible)
        float srcVal[3] = { fe, lastB, lfo };
        float direct = 0.0f, wheel = 0.0f;
        for (int s = 0; s < 3; ++s)
        {
            if (Q.modRoute[s] == 1) direct += srcVal[s] * Q.modAmt[s];
            else if (Q.modRoute[s] == 2) wheel += srcVal[s] * Q.modAmt[s];
        }
        wheel *= Q.modWheel;
        float dest[5];
        for (int d = 0; d < 5; ++d) dest[d] = (Q.modDest[d] == 1 ? direct : (Q.modDest[d] == 2 ? wheel : 0.0f));
        aFreqMod = dest[0] * 12.0f; aPwMod = dest[1] * 0.4f; bFreqMod = dest[2] * 12.0f; bPwMod = dest[3] * 0.4f; filMod = dest[4] * 5.0f;
        filMod += Q.perfCutOct;
        if (Q.perfVibSemi > 0.0005f) { const float vib = (ln > 0 ? lfo : std::sin (6.2831853f * lfoPh)) * Q.perfVibSemi; aFreqMod += vib; bFreqMod += vib; }
        if (rv != 0.0f)
        {
            if (Q.rndPA) aFreqMod += rv * 1.5f;
            if (Q.rndPB) bFreqMod += rv * 1.5f;
            if (Q.rndPW) { aPwMod += rv * 0.35f; bPwMod += rv * 0.35f; }
            if (Q.rndCut) filMod += rv * 2.0f;
        }

        freqB *= std::pow (2.0f, bFreqMod / 12.0f);
        float dtB = clampf (freqB / osr, 1.0e-6f, 0.45f);
        float pwB = clampf (Q.bPW + bPwMod, 0.06f, 0.94f);

        // advance B, detect wrap for sync
        float prevPhB = phB; phB += dtB; bool bWrapped = false; float syncFrac = 0.0f;
        if (phB >= 1.0f) { phB -= 1.0f; bWrapped = true; syncFrac = phB / dtB; }
        auto waveB = [&] (float t, float dt)
        {
            float v = 0.0f; int n = 0;
            if (Q.bSaw)   { v += naiveSaw (t) - polyBlep (t, dt); ++n; }
            if (Q.bTri)   { v += 4.0f * std::abs (t - 0.5f) - 1.0f; ++n; }
            if (Q.bPulse) { v += naivePulse (t, pwB) + polyBlep (t, dt) - polyBlep (std::fmod (t + 1.0f - pwB, 1.0f), dt); ++n; }
            if (n > 1) v *= 1.0f / std::sqrt ((float) n);
            return v;
        };
        float bv = waveB (phB, dtB);
        lastB = bv; (void) prevPhB;

        // ---- oscillator A (always keyboard controlled) ----
        float freqA = midiToHz (note + Q.aTune + (float) Q.aSemi + 12.0f * (Q.aOct - 1) + aFreqMod);
        float dtA = clampf (freqA / osr, 1.0e-6f, 0.45f);
        float pwA = clampf (Q.aPW + aPwMod, 0.06f, 0.94f);
        float phAold = phA; phA += dtA; if (phA >= 1.0f) { phA -= 1.0f; subFlip = 1.0f - subFlip; }
        auto waveA = [&] (float t, float dt)
        {
            float v = 0.0f; int n = 0;
            if (Q.aSaw)   { v += naiveSaw (t) - polyBlep (t, dt); ++n; }
            if (Q.aPulse) { v += naivePulse (t, pwA) + polyBlep (t, dt) - polyBlep (std::fmod (t + 1.0f - pwA, 1.0f), dt); ++n; }
            return n > 1 ? v * 0.7071f : v;
        };
        auto naiveA = [&] (float t)
        {
            float v = 0.0f; int n = 0;
            if (Q.aSaw)   { v += naiveSaw (t); ++n; }
            if (Q.aPulse) { v += naivePulse (t, pwA); ++n; }
            return n > 1 ? v * 0.7071f : v;
        };
        float a = waveA (phA, dtA);
        float emitA = prevA + prevPending; prevPending = 0.0f;
        if (Q.aSync && bWrapped)
        {
            float pOld = phA; // phase A would have continued from here
            float newPh = syncFrac * dtA;
            float step = naiveA (newPh) - naiveA (pOld);
            phA = newPh;
            a = waveA (phA, dtA);
            emitA += 0.5f * step * syncFrac * syncFrac;           // sample before the reset
            a     += 0.5f * step * blepAfter (syncFrac);          // sample after the reset
        }
        (void) phAold;
        prevA = a;

        // ---- unison: extra detuned copies of A and B (free running phases), summed and scaled by 1/sqrt(voices) ----
        float uniA = 0.0f, uniB = 0.0f, uniG = 1.0f;
        if (Q.uniVoices > 1)
        {
            const int ex = std::min (Q.uniVoices, 5) - 1;
            const float maxCents = 100.0f * Q.uniDetune, half = (float) ((ex + 1) / 2);
            // drift: new random targets every ~1.7 s, each copy glides to its target; depth grows with the detune knob, zero at 0
            if (uDriftCnt-- <= 0) { uDriftCnt = (int) (1.7f * osr); for (int k = 0; k < 4; ++k) uDriftT[k] = rnd(); }
            const float dSlew = 1.0f / (1.5f * osr), dCents = 4.0f * clampf (Q.uniDetune * 3.0f, 0.0f, 1.0f);
            for (int k = 0; k < ex; ++k)
            {
                uDrift[k] += dSlew * (uDriftT[k] - uDrift[k]);
                const float mag = (float) ((k + 2) / 2) / half, sgn = (k & 1) ? -1.0f : 1.0f;
                const float ratio = std::exp2 ((sgn * mag * maxCents + uDrift[k] * dCents) * (1.0f / 1200.0f));
                if (! Q.bLow) { const float dk = clampf (dtB * ratio, 1.0e-6f, 0.45f); phBx[k] += dk; if (phBx[k] >= 1.0f) phBx[k] -= 1.0f; uniB += waveB (phBx[k], dk); }
                if (! Q.aSync) { const float dk = clampf (dtA * ratio, 1.0e-6f, 0.45f); phAx[k] += dk; if (phAx[k] >= 1.0f) phAx[k] -= 1.0f; uniA += waveA (phAx[k], dk); }
            }
            uniG = 1.0f / std::sqrt ((float) (ex + 1));
        }

        // ---- sub oscillator (one octave below A, square, smoothed) ----
        // Locked to A's phase (A wraps twice per sub cycle) so the two never drift against each other and the
        // blend sounds the same on every note; follows A's pitch modulation. With hard sync it free-runs instead.
        float dtS, sub;
        if (! Q.aSync)
        {
            dtS = 0.5f * dtA; phSub = 0.5f * (phA + subFlip);
        }
        else
        {
            const float freqSub = 0.5f * midiToHz (note + Q.aTune + (float) Q.aSemi + 12.0f * (Q.aOct - 1) + aFreqMod);
            dtS = clampf (freqSub / osr, 1.0e-6f, 0.45f);
            phSub += dtS; if (phSub >= 1.0f) phSub -= 1.0f;
        }
        sub = naivePulse (phSub, 0.5f) + polyBlep (phSub, dtS) - polyBlep (std::fmod (phSub + 0.5f, 1.0f), dtS);
        subLp += 0.12f * (sub - subLp); // rounds the sub so it stays warm, not buzzy

        // ---- mixer ----
        float noise = rnd() * 0.5f;
        float mix = (emitA + uniA) * uniG * Q.mixA * 0.6f + (bv + uniB) * uniG * Q.mixB * 0.6f + noise * Q.mixNoise * 0.5f + subLp * Q.mixSub * 0.6f;

        // ---- filter: zero delay feedback 4 pole ladder, tanh on input and in the loop ----
        float oct = std::log2 (std::max (Q.cutoffHz, 10.0f)) + Q.fenvAmt * fe * 6.0f * (1.0f + 1.0f * sq) + Q.kbdAmt * (note - 60.0f) / 12.0f + filMod
                  + 2.0f * Q.accAmt * accEnv + 2.5f * sq;
        float fc = clampf (std::exp2 (oct), 15.0f, 0.45f * osr);
        float g = std::tan (kPi * fc / osr);
        float G = g / (1.0f + g);
        // resonance taper: gentle at first, strong near the top (R)
        float rz = clampf (Q.reso + 1.2f * sq, 0.0f, 0.97f > Q.reso ? 0.97f : Q.reso);
        float k = 4.4f * rz * std::sqrt (rz);
        float G4 = G * G * G * G;
        float S = G * G * G * (st[0] / (1.0f + g)) + G * G * (st[1] / (1.0f + g)) + G * (st[2] / (1.0f + g)) + st[3] / (1.0f + g);
        // drive and fuzz in front of the filter, level matched
        float x = shaper (mix, Q.drive * preAmt, Q.xstr * preAmt, Q.fuzz * preAmt) * mkQ;
        x += rnd() * 2.0e-4f * smoothstep01 ((Q.reso - 0.85f) * 6.6f);   // seed for self-oscillation only
        float u = (x - k * S) / (1.0f + k * G4);
        float v;
        float in = std::tanh (u);               // keeps resonance from spiking
        const float in0 = in; float tap[4];
        for (int i = 0; i < 4; ++i)
        {
            v = (in - st[i]) * G;
            float lp = v + st[i];
            st[i] = lp + v;
            in = lp; tap[i] = lp;
        }
        // low pass is the last stage; high pass is the same cascade subtracted from the input: (1 - H)^4 = 1 - 4H + 6H^2 - 4H^3 + H^4
        float y = in;
        if (Q.filterMode == 1) y = in0 - 4.0f * tap[0] + 6.0f * tap[1] - 4.0f * tap[2] + tap[3];
        else if (Q.filterMode == 2) y = 4.0f * (tap[1] - 2.0f * tap[2] + tap[3]);   // band pass: 4 (y2 - 2 y3 + y4)
        // grit after the filter, so the new harmonics are not filtered away again; level matched
        // transformer-style weight: a little of the low end (about 90 Hz and below) is pushed harder into the stage
        xfLp += xfC * (y - xfLp);
        const float dP = Q.drive * postAmt, xP = Q.xstr * postAmt, fP = Q.fuzz * postAmt;
        const float yd = y + 0.6f * xP * xfLp;
        y = shaper (yd * 2.0f, dP, xP, fP) * mkPQ * 0.5f * (1.0f + 0.2f * xP);
        {   // fizz control: the nastier it gets, the more the top end above ~12 kHz down to ~5 kHz is smoothed off
            const float nast = std::max (smoothstep01 ((dP - 0.8f) * 5.0f), std::max (smoothstep01 ((xP - 0.8f) * 5.0f), smoothstep01 ((fP - 0.8f) * 5.0f)));
            const float w = 6.2831853f * (14000.0f - 10000.0f * nast) / osr;
            nLp += (w / (1.0f + w)) * (y - nLp);
            y = nLp;
        }
        y *= 1.0f + 0.35f * rz;             // keep bass weight when resonance rises (R)
        y *= ae * Q.volume * 1.4f * (1.0f + 1.5f * Q.accAmt * accLvl);
        return y;
    }

    float lastB = 0.0f, subLp = 0.0f;

    void smoothParams()
    {
        const float c = 1.0f - std::exp (-1.0f / (0.004f * osr));       // about 4 ms
        auto sm = [c] (float& q, float t) { q += (t - q) * c; };
        sm (Q.aTune, P.aTune); sm (Q.bTune, P.bTune); sm (Q.aPW, P.aPW); sm (Q.bPW, P.bPW);
        sm (Q.mixA, P.mixA); sm (Q.mixB, P.mixB); sm (Q.mixNoise, P.mixNoise); sm (Q.mixSub, P.mixSub);
        sm (Q.cutoffHz, P.cutoffHz); sm (Q.reso, P.reso); sm (Q.fenvAmt, P.fenvAmt); sm (Q.kbdAmt, P.kbdAmt);
        sm (Q.lfoHz, P.lfoHz); sm (Q.modWheel, P.modWheel); sm (Q.uniDetune, P.uniDetune); sm (Q.perfCutOct, P.perfCutOct); sm (Q.perfVibSemi, P.perfVibSemi); sm (Q.drive, P.drive); sm (Q.xstr, P.xstr); sm (Q.fuzz, P.fuzz); sm (mkQ, mkT); sm (mkPQ, mkPT); sm (Q.volume, P.volume);
        for (int i = 0; i < 3; ++i) sm (Q.modAmt[i], P.modAmt[i]);
        // everything else switches directly
        Q.aOct = P.aOct; Q.aSaw = P.aSaw; Q.aPulse = P.aPulse; Q.aSync = P.aSync; Q.bOct = P.bOct; Q.bSaw = P.bSaw; Q.bTri = P.bTri; Q.bPulse = P.bPulse;
        Q.uniVoices = P.uniVoices; Q.bLow = P.bLow; Q.aSemi = P.aSemi; Q.bSemi = P.bSemi; Q.bKbd = P.bKbd; Q.fA = P.fA; Q.fD = P.fD; Q.fS = P.fS; Q.fR = P.fR; Q.aA = P.aA; Q.aD = P.aD; Q.aS = P.aS; Q.aR = P.aR;
        Q.glide = P.glide; Q.glideAuto = P.glideAuto; Q.lfoSaw = P.lfoSaw; Q.lfoTri = P.lfoTri; Q.lfoPulse = P.lfoPulse;
        for (int i = 0; i < 3; ++i) Q.modRoute[i] = P.modRoute[i];
        for (int i = 0; i < 5; ++i) Q.modDest[i] = P.modDest[i];
        Q.squelch = P.squelch; Q.squelchAmt = P.squelchAmt; Q.accAmt = P.accAmt; Q.rndHz = P.rndHz; Q.rndSlew = P.rndSlew; Q.rndDepth = P.rndDepth;
        Q.rndCut = P.rndCut; Q.rndPA = P.rndPA; Q.rndPB = P.rndPB; Q.rndPW = P.rndPW;
        Q.retrig = P.retrig; Q.repeat = P.repeat; Q.drone = P.drone; Q.filterMode = P.filterMode; Q.colourPos = P.colourPos;
    }

    static float midiToHz (float n) { return 440.0f * std::exp2 ((n - 69.0f) / 12.0f); }
};

} // namespace purrone
