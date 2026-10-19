// PurrOne effects: 3 band EQ, short high-passed reverb, gentle multiband maximizer, lookahead safety limiter.
// Header only, no JUCE types, no allocation after prepare().
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace purrone {

constexpr double kTwoPi = 6.283185307179586476925286766559;

// ---------- RBJ biquad (Audio EQ Cookbook) ----------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void set (double bb0, double bb1, double bb2, double aa0, double aa1, double aa2)
    { b0 = bb0 / aa0; b1 = bb1 / aa0; b2 = bb2 / aa0; a1 = aa1 / aa0; a2 = aa2 / aa0; }

    void lowpass (double sr, double f, double q)
    { double w = kTwoPi * f / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a); }
    void highpass (double sr, double f, double q)
    { double w = kTwoPi * f / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a); }
    void bandpass (double sr, double f, double q)
    { double w = kTwoPi * f / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set (a, 0, -a, 1 + a, -2 * c, 1 - a); }
    void peak (double sr, double f, double q, double gainDb)
    { double A = std::pow (10.0, gainDb / 40), w = kTwoPi * f / sr, c = std::cos (w), a = std::sin (w) / (2 * q);
      set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A); }
    void lowShelf (double sr, double f, double gainDb)
    { double A = std::pow (10.0, gainDb / 40), w = kTwoPi * f / sr, c = std::cos (w), s = std::sin (w), al = s / 2 * std::sqrt (2.0), t = 2 * std::sqrt (A) * al;
      set (A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t), (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t); }
    void highShelf (double sr, double f, double gainDb)
    { double A = std::pow (10.0, gainDb / 40), w = kTwoPi * f / sr, c = std::cos (w), s = std::sin (w), al = s / 2 * std::sqrt (2.0), t = 2 * std::sqrt (A) * al;
      set (A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t), (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t); }

    float process (float x)   // transposed direct form II
    { double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
};

// ---------- 3 band EQ: low shelf 100 Hz, bell 1 kHz, high shelf 6 kHz ----------
struct Eq3
{
    Biquad lo, mid, hi;
    double sr = 48000, gl = 1e9, gm = 1e9, gh = 1e9;
    void prepare (double s) { sr = s; gl = gm = gh = 1e9; lo.reset(); mid.reset(); hi.reset(); }
    void setGains (double l, double m, double h)
    {
        if (l != gl) { gl = l; lo.lowShelf (sr, 100.0, l); }
        if (m != gm) { gm = m; mid.peak (sr, 1000.0, 0.8, m); }
        if (h != gh) { gh = h; hi.highShelf (sr, 6000.0, h); }
    }
    float process (float x) { return hi.process (mid.process (lo.process (x))); }
};

// ---------- reverb: 8 line feedback delay network, short and clean by design ----------
class Reverb
{
public:
    void prepare (double s)
    {
        sr = s;
        static const int base[8] = { 1117, 1327, 1559, 1693, 1861, 2053, 2309, 2539 };   // mutually prime lengths at 48 kHz
        for (int i = 0; i < 8; ++i)
        {
            len[i] = (int) (base[i] * sr / 48000.0);
            line[i].assign ((size_t) len[i] + 64, 0.0f); pos[i] = 0; lp[i] = 0;
            lfoPh[i] = 0.13f * (float) i;
        }
        pre.assign ((size_t) (sr * 0.12) + 8, 0.0f); prePos = 0; preSamples = (int) (sr * 0.012);
        for (int i = 0; i < 4; ++i) { ap[i].assign ((size_t) (sr * apT[i]) + 2, 0.0f); apPos[i] = 0; }
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
        setParams (0.9f, 250.0f);
    }
    void setParams (float rt60Seconds, float highpassHz)
    {
        rt = std::max (0.1f, rt60Seconds);
        for (int i = 0; i < 8; ++i) gain[i] = std::pow (10.0f, -3.0f * (float) len[i] / ((float) sr * rt));
        hpL.highpass (sr, highpassHz, 0.707); hpR.highpass (sr, highpassHz, 0.707);
        hpL2.highpass (sr, highpassHz, 0.707); hpR2.highpass (sr, highpassHz, 0.707);
        lpL.lowpass (sr, 7500.0, 0.707); lpR.lowpass (sr, 7500.0, 0.707);
        damp = dampCoef;  // one pole damping coefficient, softens the tail
    }
    // pre-delay in ms (default 12), damping 0..1 (default 0.5 = the classic setting), width 0..1 (default 0.8 = the classic spread)
    void setExtras (float preMs, float dampUi, float widthUi)
    {
        preSamples = std::min ((int) pre.size() - 2, std::max (0, (int) (sr * 0.001 * preMs)));
        dampCoef = std::min (1.0f, std::max (0.1f, 0.55f + (0.5f - dampUi) * 0.9f)); damp = dampCoef;
        widthF = std::min (1.0f, std::max (0.0f, widthUi)) / 0.8f;
    }
    // wet only, stereo
    void process (float in, float& outL, float& outR)
    {
        pre[(size_t) prePos] = in; int rp = prePos - preSamples; if (rp < 0) rp += (int) pre.size(); prePos = (prePos + 1) % (int) pre.size();
        float x = pre[(size_t) rp];
        for (int i = 0; i < 4; ++i) { float d = ap[i][(size_t) apPos[i]]; float v = x + 0.6f * d; ap[i][(size_t) apPos[i]] = v; x = d - 0.6f * v; if (++apPos[i] >= (int) ap[i].size()) apPos[i] = 0; }
        float y[8], sum = 0;
        for (int i = 0; i < 8; ++i)
        {
            lfoPh[i] += 0.3f * (1.0f + 0.1f * (float) i) / (float) sr; if (lfoPh[i] > 1.0f) lfoPh[i] -= 1.0f;
            float mod = 1.5f * std::sin (6.2831853f * lfoPh[i]);                    // about 1.5 samples deep, 0.3 Hz and up
            float rpos = (float) pos[i] - (float) len[i] + mod; while (rpos < 0) rpos += (float) line[i].size();
            int i0 = (int) rpos; float fr = rpos - (float) i0; int i1 = (i0 + 1) % (int) line[i].size(); i0 %= (int) line[i].size();
            y[i] = line[i][(size_t) i0] * (1 - fr) + line[i][(size_t) i1] * fr;
            sum += y[i];
        }
        float h = sum * (2.0f / 8.0f);                                          // Householder feedback matrix
        float l = 0, r = 0;
        for (int i = 0; i < 8; ++i)
        {
            float fb = (y[i] - h) * gain[i];
            lp[i] += damp * (fb - lp[i]);                                           // one pole damping in the loop
            line[i][(size_t) pos[i]] = x * 0.35f + lp[i];
            pos[i] = (pos[i] + 1) % (int) line[i].size();
            if (i & 1) r += y[i]; else l += y[i];
        }
        l *= 0.5f; r *= 0.5f;
        outL = lpL.process (hpL2.process (hpL.process (l)));
        outR = lpR.process (hpR2.process (hpR.process (r)));
        if (widthF != 1.0f) { const float m = 0.5f * (outL + outR), sd = 0.5f * (outL - outR) * widthF; outL = m + sd; outR = m - sd; }
    }
private:
    double sr = 48000; float rt = 0.9f, damp = 0.3f, dampCoef = 0.55f, widthF = 1.0f; int preSamples = 576;
    int len[8] {}, pos[8] {}; float gain[8] {}, lp[8] {}, lfoPh[8] {};
    std::vector<float> line[8], pre; int prePos = 0;
    static constexpr double apT[4] = { 0.0047, 0.0071, 0.0103, 0.0149 };
    std::vector<float> ap[4]; int apPos[4] {};
    Biquad hpL, hpR, hpL2, hpR2, lpL, lpR;
};

// ---------- 4th order Linkwitz-Riley crossover pieces ----------
struct LR4
{
    Biquad a, b;
    void lowpass (double sr, double f)  { a.lowpass (sr, f, 0.70710678); b = a; }
    void highpass (double sr, double f) { a.highpass (sr, f, 0.70710678); b = a; }
    float process (float x) { return b.process (a.process (x)); }
    void reset() { a.reset(); b.reset(); }
};

// ---------- gentle three band "maximizer": downward and upward compression, slow, no pumping ----------
class Maximizer
{
public:
    void prepare (double s)
    {
        sr = s;
        for (int c = 0; c < 2; ++c)
        {
            lp1[c].lowpass (sr, 120.0);   hp1[c].highpass (sr, 120.0);
            lp2[c].lowpass (sr, 2500.0);  hp2[c].highpass (sr, 2500.0);
            apLo[c].lowpass (sr, 2500.0); apLoH[c].highpass (sr, 2500.0);
        }
        for (auto& e : env) e = -60.0f;
        for (auto& g : gdb) g = 0.0f;
        atk = 1.0f - std::exp (-1.0f / (0.012f * (float) sr)); rel = 1.0f - std::exp (-1.0f / (0.18f * (float) sr));
    }
    void setAmount (float a) { amount = std::min (1.0f, std::max (0.0f, a)); }
    float amountNow() const { return amount; }

    void process (float& l, float& r)
    {
        if (amount < 0.0005f) { // keep crossover state warm so switching on is click free, but pass the input through
            splitAndSum (l, r, nullptr); return;
        }
        float bands[3][2]; splitAndSum (l, r, bands);
        // per band gain from linked stereo level
        static const float hiThr[3] = { -14.0f, -16.0f, -18.0f }, loThr[3] = { -34.0f, -36.0f, -38.0f };
        float outL = 0, outR = 0;
        for (int b = 0; b < 3; ++b)
        {
            float lev = std::max (std::fabs (bands[b][0]), std::fabs (bands[b][1])), ldb = 20.0f * std::log10 (lev + 1.0e-9f);
            float c = ldb > env[b] ? atk : rel; env[b] += c * (ldb - env[b]);
            float down = 0.0f, up = 0.0f;
            if (env[b] > hiThr[b]) down = -(env[b] - hiThr[b]) * (1.0f - 1.0f / (1.0f + 4.0f * amount));      // up to 5:1 at full
            if (env[b] < loThr[b]) up = std::min (9.0f * amount, (loThr[b] - env[b]) * (1.0f - 1.0f / (1.0f + 3.0f * amount)));   // lift quiet parts, capped
            float target = down + up;
            gdb[b] += 0.0035f * (target - gdb[b]);                                                          // extra smoothing so gain never steps
            float g = std::pow (10.0f, gdb[b] / 20.0f);
            outL += bands[b][0] * g; outR += bands[b][1] * g;
        }
        float mk = std::pow (10.0f, (3.5f * amount) / 20.0f);          // small makeup
        l = outL * mk; r = outR * mk;
    }
private:
    double sr = 48000; float amount = 0, atk = 0.01f, rel = 0.001f, env[3] {}, gdb[3] {};
    LR4 lp1[2], hp1[2], lp2[2], hp2[2], apLo[2], apLoH[2];
    void splitAndSum (float& l, float& r, float (*bands)[2])
    {
        float in[2] = { l, r }, lo[2], mid[2], hi[2];
        for (int c = 0; c < 2; ++c)
        {
            lo[c] = lp1[c].process (in[c]);
            float rest = hp1[c].process (in[c]);
            mid[c] = lp2[c].process (rest);
            hi[c]  = hp2[c].process (rest);
            lo[c]  = apLo[c].process (lo[c]) + apLoH[c].process (lo[c]);     // all-pass at the upper crossover keeps the sum flat
        }
        if (bands) for (int c = 0; c < 2; ++c) { bands[0][c] = lo[c]; bands[1][c] = mid[c]; bands[2][c] = hi[c]; }
        else { l = lo[0] + mid[0] + hi[0]; r = lo[1] + mid[1] + hi[1]; }
    }
};

// ---------- lookahead safety limiter: min filter, release smoothing, boxcar. Guarantees the ceiling. ----------
class SafetyLimiter
{
public:
    void prepare (double sr, float ceilingDb = -1.0f)
    {
        L = std::max (8, (int) (sr * 0.002)); ceiling = std::pow (10.0f, ceilingDb / 20.0f);
        dl[0].assign ((size_t) L + 1, 0.0f); dl[1].assign ((size_t) L + 1, 0.0f); gRing.assign ((size_t) L, 1.0f); mRing.assign ((size_t) L, 1.0f);
        wp = 0; bi = 0; prevM = 1.0f; boxSum = (float) L; rel = 1.0f - std::exp (-1.0f / (0.08f * (float) sr));
        hist.assign ((size_t) L, 1.0f); hp = 0;
    }
    int latency() const { return L - 1; }
    void setEnabled (bool e) { on = e; }
    void setCeiling (float db) { ceiling = std::pow (10.0f, std::min (0.0f, std::max (-12.0f, db)) / 20.0f); }
    void process (float& l, float& r)
    {
        // delay line of the audio, so gain computed from the future applies to the right sample
        dl[0][(size_t) wp] = l; dl[1][(size_t) wp] = r;
        float pk = std::max (std::fabs (l), std::fabs (r));
        float g = (on && pk > ceiling) ? ceiling / pk : 1.0f;
        hist[(size_t) hp] = g; hp = (hp + 1) % L;
        float m = 1.0f; for (int i = 0; i < L; ++i) m = std::min (m, hist[(size_t) i]);          // min over the lookahead window
        float ms = std::min (m, prevM + (1.0f - prevM) * rel); prevM = ms;                      // instant down, slow release
        boxSum += ms - gRing[(size_t) bi]; gRing[(size_t) bi] = ms; bi = (bi + 1) % L;   // boxcar smooths the attack over L samples
        float gain = boxSum / (float) L;
        int rp = wp - (L - 1); if (rp < 0) rp += L + 1;
        l = dl[0][(size_t) rp] * gain; r = dl[1][(size_t) rp] * gain;
        wp = (wp + 1) % (L + 1);
        if (! on) { l = dl[0][(size_t) rp]; r = dl[1][(size_t) rp]; }
    }
private:
    int L = 96, wp = 0, hp = 0, bi = 0; float ceiling = 0.89f, prevM = 1, boxSum = 96, rel = 0.0003f; bool on = true;
    std::vector<float> dl[2], gRing, mRing, hist;
};


// LA-2A style optical leveller: soft knee, two-stage program-dependent release, gentle makeup. Linked stereo.
struct Opto
{
    void prepare (double sampleRate) { fs = (float) sampleRate; reset(); }
    void reset() { gr1 = gr2 = mem = env = 0.0f; }
    void setAmount (float a) { amount = std::min (1.0f, std::max (0.0f, a)); }
    void setSpeed (int s) { sm = s <= 0 ? 2.0f : (s == 1 ? 1.0f : 0.4f); }   // 0 slow, 1 medium (the original setting), 2 fast
    void process (float& l, float& r)
    {
        if (amount < 0.0005f) { gr1 = gr2 = mem = env = 0.0f; return; }
        const float p = amount;
        const float peak = std::max (std::fabs (l), std::fabs (r));
        const float k = 1.0f - std::exp (-1.0f / (0.0015f * fs));
        env += k * (peak - env);
        const float db = 20.0f * std::log10 (std::max (env, 1.0e-5f));
        const float thr = -6.0f - 30.0f * p, knee = 12.0f, over = db - thr;
        float target = 0.0f;
        if (over > knee * 0.5f) target = over * (2.0f / 3.0f);
        else if (over > -knee * 0.5f) { const float t = over + knee * 0.5f; target = (2.0f / 3.0f) * t * t / (2.0f * knee); }
        const float att = 1.0f - std::exp (-1.0f / (0.010f * sm * fs));
        const float rel1 = 1.0f - std::exp (-1.0f / (0.0866f * sm * fs));
        const float rel2 = 1.0f - std::exp (-1.0f / ((1.0f + 4.0f * mem) * sm * fs));
        gr1 += (target > gr1 ? att : rel1) * (target - gr1);
        gr2 += (target > gr2 ? att : rel2) * (target - gr2);
        mem += (1.0f - std::exp (-1.0f / (2.0f * fs))) * (std::min (1.0f, std::max (0.0f, 0.5f * (gr1 + gr2) / 15.0f)) - mem);
        const float g = std::pow (10.0f, (-0.5f * (gr1 + gr2) + 6.0f * p) / 20.0f);
        l *= g; r *= g;
    }
private:
    float sm = 1.0f, fs = 48000.0f, amount = 0.0f, gr1 = 0.0f, gr2 = 0.0f, mem = 0.0f, env = 0.0f;
};

} // namespace purrone
