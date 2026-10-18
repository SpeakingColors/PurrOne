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
    int   aOct = 1;   float aTune = 0.0f;  bool aSaw = true, aPulse = false; float aPW = 0.5f; bool aSync = false;
    // Oscillator B
    int   bOct = 1;   float bTune = 0.0f;  bool bSaw = true, bTri = false, bPulse = false; float bPW = 0.5f;
    bool  bLow = false, bKbd = true;
    // Mixer 0..1
    float mixA = 0.8f, mixB = 0.0f, mixNoise = 0.0f, mixSub = 0.0f;
    // Filter
    float cutoffHz = 3000.0f, reso = 0.1f, fenvAmt = 0.0f, kbdAmt = 0.5f; // fenvAmt -1..1
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
    // Keyboard
    bool retrig = false, repeat = false, drone = false;
    // Output
    float drive = 0.0f;   // 0..1 pre-filter warmth
    float volume = 0.7f;
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate; osr = sr * 2.0f;
        for (auto* e : { &fenv, &aenv }) e->sr = osr;
        buildHalfband();
        reset();
    }

    void reset()
    {
        phA = phB = phSub = 0.0f; fenv.reset(); aenv.reset();
        for (auto& s : st) s = 0.0f;
        lfoPh = 0.0f; hbPos = 0; hbBuf.fill (0.0f); heldCount = 0; curNote = -1; glidePitch = 60.0f;
        prevA = 0.0f; gateOn = false; repeatPh = 0.0f; seed = 22222u; dcX = dcY = 0.0f;
    }

    void setParams (const Params& p) { P = p; }

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

    void allNotesOff() { heldCount = 0; gateOn = false; fenv.gateOff(); aenv.gateOff(); }
    bool isActive() const { return aenv.active() || gateOn || P.drone; }

    // mono output, n samples
    void process (float* out, int n)
    {
        applyEnvParams();
        for (int i = 0; i < n; ++i)
        {
            float y0 = tick2x(), y1 = tick2x();
            out[i] = decimate (y0, y1);
        }
    }

private:
    static constexpr int kMaxHeld = 16;
    Params P;
    float sr = 48000.0f, osr = 96000.0f;
    float phA = 0, phB = 0, phSub = 0, lfoPh = 0, prevA = 0, repeatPh = 0;
    float st[4] = { 0, 0, 0, 0 };
    Adsr fenv, aenv;
    int held[kMaxHeld] {}; int heldCount = 0, curNote = -1; bool gateOn = false;
    float glidePitch = 60.0f;
    uint32_t seed = 22222u;
    float dcX = 0, dcY = 0;
    // pending sync correction for the delayed osc A sample
    float prevPending = 0.0f;

    // halfband decimator
    static constexpr int kTaps = 95;
    std::array<float, kTaps> hb {};
    std::array<float, kTaps> hbBuf {};
    int hbPos = 0;

    void buildHalfband()
    {
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
        aenv.a = Adsr::timeFromKnob (P.aA); aenv.d = Adsr::timeFromKnob (P.aD); aenv.s = P.aS; aenv.r = Adsr::timeFromKnob (P.aR);
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
        const float dtSec = 1.0f / osr;
        // ---- glide (exponential slew in semitones) ----
        float gt = P.glide <= 0.001f ? 0.0f : 0.005f * std::pow (800.0f, P.glide); // 5 ms .. 4 s
        if (curNote >= 0)
        {
            if (gt <= 0.0f) glidePitch = (float) curNote;
            else glidePitch += ((float) curNote - glidePitch) * (1.0f - std::exp (-dtSec / gt));
        }
        const float note = glidePitch;

        // ---- envelopes and repeat ----
        if (P.repeat && gateOn && ! P.drone)
        {
            repeatPh += P.lfoHz * dtSec;
            if (repeatPh >= 1.0f) { repeatPh -= 1.0f; fenv.gateOn(); aenv.gateOn(); }
            else if (repeatPh > 0.5f) { fenv.gateOff(); aenv.gateOff(); }
        }
        if (P.drone && ! aenv.active()) { fenv.gateOn(); aenv.gateOn(); }
        float fe = fenv.tick(), ae = aenv.tick();

        // ---- LFO (mix of enabled shapes, scaled so a single shape peaks at +-1) ----
        lfoPh += P.lfoHz * dtSec; if (lfoPh >= 1.0f) lfoPh -= 1.0f;
        float lfo = 0.0f; int ln = 0;
        if (P.lfoSaw)   { lfo += naiveSaw (lfoPh); ++ln; }
        if (P.lfoTri)   { lfo += 4.0f * std::abs (lfoPh - 0.5f) - 1.0f; ++ln; }
        if (P.lfoPulse) { lfo += naivePulse (lfoPh, 0.5f); ++ln; }
        if (ln > 1) lfo /= (float) ln;

        // ---- oscillator B ----
        float freqB;
        if (P.bLow) freqB = (P.bKbd ? 0.25f * midiToHz (note + P.bTune) : 0.25f * 8.0f) * 0.0625f; // sub-audio
        else        freqB = midiToHz ((P.bKbd ? note : 60.0f) + P.bTune + 12.0f * (P.bOct - 1));
        float bFreqMod = 0.0f, bPwMod = 0.0f, aFreqMod = 0.0f, aPwMod = 0.0f, filMod = 0.0f;

        // modulation sources use the previous B sample (one tick delay is inaudible)
        float srcVal[3] = { fe, lastB, lfo };
        float direct = 0.0f, wheel = 0.0f;
        for (int s = 0; s < 3; ++s)
        {
            if (P.modRoute[s] == 1) direct += srcVal[s] * P.modAmt[s];
            else if (P.modRoute[s] == 2) wheel += srcVal[s] * P.modAmt[s];
        }
        wheel *= P.modWheel;
        float dest[5];
        for (int d = 0; d < 5; ++d) dest[d] = (P.modDest[d] == 1 ? direct : (P.modDest[d] == 2 ? wheel : 0.0f));
        aFreqMod = dest[0] * 12.0f; aPwMod = dest[1] * 0.4f; bFreqMod = dest[2] * 12.0f; bPwMod = dest[3] * 0.4f; filMod = dest[4] * 5.0f;

        freqB *= std::pow (2.0f, bFreqMod / 12.0f);
        float dtB = clampf (freqB / osr, 1.0e-6f, 0.45f);
        float pwB = clampf (P.bPW + bPwMod, 0.06f, 0.94f);

        // advance B, detect wrap for sync
        float prevPhB = phB; phB += dtB; bool bWrapped = false; float syncFrac = 0.0f;
        if (phB >= 1.0f) { phB -= 1.0f; bWrapped = true; syncFrac = phB / dtB; }
        float bv = 0.0f; int bn = 0;
        if (P.bSaw)   { bv += naiveSaw (phB) - polyBlep (phB, dtB); ++bn; }
        if (P.bTri)   { bv += 4.0f * std::abs (phB - 0.5f) - 1.0f; ++bn; }
        if (P.bPulse) { bv += naivePulse (phB, pwB) + polyBlep (phB, dtB) - polyBlep (std::fmod (phB + 1.0f - pwB, 1.0f), dtB); ++bn; }
        if (bn > 1) bv *= 1.0f / std::sqrt ((float) bn);
        lastB = bv; (void) prevPhB;

        // ---- oscillator A (always keyboard controlled) ----
        float freqA = midiToHz (note + P.aTune + 12.0f * (P.aOct - 1) + aFreqMod);
        float dtA = clampf (freqA / osr, 1.0e-6f, 0.45f);
        float pwA = clampf (P.aPW + aPwMod, 0.06f, 0.94f);
        float phAold = phA; phA += dtA; if (phA >= 1.0f) phA -= 1.0f;
        auto waveA = [&] (float t, float dt)
        {
            float v = 0.0f; int n = 0;
            if (P.aSaw)   { v += naiveSaw (t) - polyBlep (t, dt); ++n; }
            if (P.aPulse) { v += naivePulse (t, pwA) + polyBlep (t, dt) - polyBlep (std::fmod (t + 1.0f - pwA, 1.0f), dt); ++n; }
            return n > 1 ? v * 0.7071f : v;
        };
        auto naiveA = [&] (float t)
        {
            float v = 0.0f; int n = 0;
            if (P.aSaw)   { v += naiveSaw (t); ++n; }
            if (P.aPulse) { v += naivePulse (t, pwA); ++n; }
            return n > 1 ? v * 0.7071f : v;
        };
        float a = waveA (phA, dtA);
        float emitA = prevA + prevPending; prevPending = 0.0f;
        if (P.aSync && bWrapped)
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

        // ---- sub oscillator (one octave below A, square, smoothed) ----
        float freqSub = 0.5f * midiToHz (note + P.aTune + 12.0f * (P.aOct - 1));
        float dtS = clampf (freqSub / osr, 1.0e-6f, 0.45f);
        phSub += dtS; if (phSub >= 1.0f) phSub -= 1.0f;
        float sub = naivePulse (phSub, 0.5f) + polyBlep (phSub, dtS) - polyBlep (std::fmod (phSub + 0.5f, 1.0f), dtS);
        subLp += 0.12f * (sub - subLp); // rounds the sub so it stays warm, not buzzy

        // ---- mixer ----
        float noise = rnd() * 0.5f;
        float mix = emitA * P.mixA * 0.6f + bv * P.mixB * 0.6f + noise * P.mixNoise * 0.5f + subLp * P.mixSub * 0.6f;

        // ---- filter: zero delay feedback 4 pole ladder, tanh on input and in the loop ----
        float oct = std::log2 (std::max (P.cutoffHz, 10.0f)) + P.fenvAmt * fe * 6.0f + P.kbdAmt * (note - 60.0f) / 12.0f + filMod;
        float fc = clampf (std::exp2 (oct), 15.0f, 0.45f * osr);
        float g = std::tan (kPi * fc / osr);
        float G = g / (1.0f + g);
        float k = P.reso * 4.4f;
        float G4 = G * G * G * G;
        float S = G * G * G * (st[0] / (1.0f + g)) + G * G * (st[1] / (1.0f + g)) + G * (st[2] / (1.0f + g)) + st[3] / (1.0f + g);
        // drive in front of the filter, soft and symmetric
        float driveGain = 1.0f + P.drive * 3.0f;
        float x = std::tanh (mix * driveGain) + rnd() * 2.0e-4f;
        float u = (x - k * S) / (1.0f + k * G4);
        float v;
        float in = std::tanh (u);               // keeps resonance from spiking
        for (int i = 0; i < 4; ++i)
        {
            v = (in - st[i]) * G;
            float lp = v + st[i];
            st[i] = lp + v;
            in = lp;
        }
        float y = in;
        y *= 1.0f + 0.35f * P.reso;             // keep bass weight when resonance rises (R)
        y *= ae * P.volume * 1.4f;
        return y;
    }

    float lastB = 0.0f, subLp = 0.0f;

    static float midiToHz (float n) { return 440.0f * std::exp2 ((n - 69.0f) / 12.0f); }
};

} // namespace purrone
