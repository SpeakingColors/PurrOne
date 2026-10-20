// PurrOne effects, part 2: 5 band dynamic EQ with mid/side targets, bit crusher, OTT, Dance master, stereo tool.
// Header only, no JUCE types, no allocation after construction. Everything is neutral (bit exact bypass) when off.
#pragma once
#include "Fx.h"
#include <atomic>

namespace purrone {

inline float dbToLin (float d) { return std::pow (10.0f, d / 20.0f); }
inline float linToDb (float x) { return 20.0f * std::log10 (x + 1.0e-9f); }
inline float clampf2 (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

// ---------- 5 band EQ. Band 1 low shelf, 2 to 4 bells, 5 high shelf. Each band: on, freq, gain, Q, dynamic, threshold, target ----------
// target 0 = Stereo (works on the mono synth signal before the space), 1 = Mid, 2 = Side (work on the stereo result).
// Dynamic: 0 keeps the band static. Positive: the band boosts by up to that many dB when the signal in the band is above the
// threshold. Negative: it cuts by up to that many dB. This is added to the static gain, in either direction, whatever the gain is.
// Full effect is reached 12 dB above the threshold.
class Eq5
{
public:
    Eq5() { for (auto& l : liveLvl) l.store (-90.0f); }
    void prepare (double s)
    {
        sr = s;
        for (int i = 0; i < 5; ++i) { flt[i].reset(); det[i].reset(); env[i] = 0.0f; amt[i] = 0.0f; dirty[i] = true; curGain[i] = 1.0e9f; }
        atk = 1.0f - std::exp (-1.0f / (0.005f * (float) sr)); rel = 1.0f - std::exp (-1.0f / (0.12f * (float) sr));
    }
    void setBand (int i, bool on, float f, float gainDb, float q, float dynDb, float thrDb, int target)
    {
        Band& b = band[i];
        if (b.f != f || b.g != gainDb || b.q != q || b.dyn != dynDb) dirty[i] = true;
        if (b.f != f || b.q != q) detDirty[i] = true;
        b.on = on; b.f = f; b.g = gainDb; b.q = q; b.dyn = dynDb; b.thr = thrDb; b.target = target;
    }
    // for the display: the dB the dynamic part is adding right now, and the detector level in dBFS (valid while the band is dynamic)
    float liveDynDb (int i) const { return liveDyn[i].load (std::memory_order_relaxed); }
    float liveLevelDb (int i) const { return liveLvl[i].load (std::memory_order_relaxed); }
    bool anyMono() const { for (auto& b : band) if (active (b) && b.target == 0) return true; return false; }
    bool anyStereo() const { for (auto& b : band) if (active (b) && b.target != 0) return true; return false; }

    float processMono (float x)
    {
        for (int i = 0; i < 5; ++i)
            if (band[i].target == 0 && active (band[i])) x = run (i, x, x);
        return x;
    }
    void processStereo (float& l, float& r)
    {
        for (int i = 0; i < 5; ++i)
        {
            const Band& b = band[i];
            if (b.target == 0 || ! active (b)) continue;
            if (b.target == 1) { const float m = 0.5f * (l + r), d = run (i, m, m) - m; l += d; r += d; }
            else               { const float sd = 0.5f * (l - r), d = run (i, sd, sd) - sd; l += d; r -= d; }
        }
    }
private:
    struct Band { bool on = true; float f = 1000.0f, g = 0.0f, q = 0.8f, dyn = 0.0f, thr = -24.0f; int target = 0; };
    static bool active (const Band& b) { return b.on && (std::fabs (b.g) > 0.005f || std::fabs (b.dyn) > 0.005f); }

    float run (int i, float x, float detIn)
    {
        const Band& b = band[i];
        if (detDirty[i]) { det[i].bandpass (sr, std::min (b.f, (float) (0.45 * sr)), std::max (0.3f, b.q)); detDirty[i] = false; }
        float g = b.g;
        if (std::fabs (b.dyn) > 0.005f)
        {
            const float y = std::fabs (det[i].process (detIn));
            env[i] += (y > env[i] ? atk : rel) * (y - env[i]);
            const float over = clampf2 ((linToDb (env[i]) - b.thr) / 12.0f, 0.0f, 1.0f);
            amt[i] += 0.02f * (over - amt[i]);
            g = clampf2 (b.g + b.dyn * amt[i], -24.0f, 24.0f);
            if (std::fabs (g - curGain[i]) > 0.02f) dirty[i] = true;
        }
        const bool dynOn = std::fabs (b.dyn) > 0.005f;
        if (dynOn && ((++cnt[i] & 31) == 0)) { liveDyn[i].store (g - b.g, std::memory_order_relaxed); liveLvl[i].store (linToDb (env[i]), std::memory_order_relaxed); }
        if (dirty[i] && (! dynOn || (cnt[i] & 31) == 0 || curGain[i] > 1.0e8f))
        {
            curGain[i] = g; dirty[i] = false;
            const double f = std::min ((double) b.f, 0.45 * sr);
            if (i == 0) flt[i].lowShelf (sr, f, g);
            else if (i == 4) flt[i].highShelf (sr, f, g);
            else flt[i].peak (sr, f, b.q, g);
        }
        return flt[i].process (x);
    }

    double sr = 48000.0; Band band[5]; Biquad flt[5], det[5]; float env[5] {}, amt[5] {}, curGain[5] {};
    std::atomic<float> liveDyn[5] {}, liveLvl[5] {};
    bool dirty[5] {}, detDirty[5] { true, true, true, true, true }; float atk = 0.01f, rel = 0.001f; unsigned cnt[5] {};
};

// ---------- bit and sample rate crusher ----------
struct Crush
{
    void prepare (double s) { sr = s; ph = 0.0; held = 0.0f; }
    void set (bool on_, float bits_, float rateHz, float mix_) { on = on_; bits = bits_; rate = rateHz; mix = mix_; }
    float process (float x)
    {
        if (! on || mix < 0.001f || (bits >= 15.99f && rate >= 0.999 * sr)) return x;
        ph += rate / sr; if (ph >= 1.0) { ph -= std::floor (ph); held = x; }
        const float q = std::exp2 (std::round (bits) - 1.0f);
        const float y = std::round (held * q) / q;
        return x + (y - x) * mix;
    }
    bool on = false; float bits = 16.0f, mix = 0.5f; double rate = 48000.0, sr = 48000.0, ph = 0.0; float held = 0.0f;
};

// ---------- three band splitter (Linkwitz-Riley), flat sum ----------
struct Split3
{
    LR4 lp1[2], hp1[2], lp2[2], hp2[2], apLo[2], apLoH[2];
    void prepare (double sr, double f1 = 120.0, double f2 = 2500.0)
    {
        for (int c = 0; c < 2; ++c)
        {
            lp1[c].lowpass (sr, f1); hp1[c].highpass (sr, f1); lp2[c].lowpass (sr, f2); hp2[c].highpass (sr, f2);
            apLo[c].lowpass (sr, f2); apLoH[c].highpass (sr, f2);
            lp1[c].reset(); hp1[c].reset(); lp2[c].reset(); hp2[c].reset(); apLo[c].reset(); apLoH[c].reset();
        }
    }
    void split (float l, float r, float (&bands)[3][2])
    {
        const float in[2] = { l, r };
        for (int c = 0; c < 2; ++c)
        {
            float lo = lp1[c].process (in[c]);
            const float rest = hp1[c].process (in[c]);
            lo = apLo[c].process (lo) + apLoH[c].process (lo);
            bands[0][c] = lo; bands[1][c] = lp2[c].process (rest); bands[2][c] = hp2[c].process (rest);
        }
    }
};

// ---------- OTT: three band upward and downward compression ----------
class Ott
{
public:
    void prepare (double s) { sr = s; split.prepare (sr); for (auto& e : env) e = -80.0f; for (auto& g : gdb) g = 0.0f; setTime (0.4f); }
    void set (bool on_, float amount_, float depth_, float time_, float loDb, float midDb, float hiDb)
    {
        on = on_; amount = clampf2 (amount_, 0.0f, 1.0f); depth = clampf2 (depth_, 0.0f, 1.0f); setTime (time_);
        trim[0] = dbToLin (loDb); trim[1] = dbToLin (midDb); trim[2] = dbToLin (hiDb);
    }
    void process (float& l, float& r)
    {
        if (! on || amount < 0.001f) { float b[3][2]; split.split (l, r, b); return; }   // keep the filters warm
        float b[3][2]; split.split (l, r, b);
        const float ratioD = 1.0f + 7.0f * depth, ratioU = 1.0f + 3.0f * depth;
        static const float thrD[3] = { -17.0f, -19.0f, -21.0f }, thrU[3] = { -34.0f, -36.0f, -38.0f };
        float wl = 0.0f, wr = 0.0f;
        for (int k = 0; k < 3; ++k)
        {
            const float lev = linToDb (std::max (std::fabs (b[k][0]), std::fabs (b[k][1])));
            env[k] += (lev > env[k] ? atk : rel) * (lev - env[k]);
            float down = 0.0f, up = 0.0f;
            if (env[k] > thrD[k]) down = -(env[k] - thrD[k]) * (1.0f - 1.0f / ratioD);
            if (env[k] < thrU[k]) up = std::min (18.0f * depth, (thrU[k] - env[k]) * (1.0f - 1.0f / ratioU));
            gdb[k] += 0.004f * (down + up - gdb[k]);
            const float g = dbToLin (gdb[k]) * trim[k];
            wl += b[k][0] * g; wr += b[k][1] * g;
        }
        const float mk = dbToLin (2.5f * depth);
        l += (wl * mk - l) * amount; r += (wr * mk - r) * amount;
    }
private:
    void setTime (float t)
    {
        t = clampf2 (t, 0.0f, 1.0f);
        atk = 1.0f - std::exp (-1.0f / ((0.001f + 0.019f * t) * (float) sr)); rel = 1.0f - std::exp (-1.0f / ((0.04f + 0.26f * t) * (float) sr));
    }
    double sr = 48000.0; bool on = false; float amount = 0.5f, depth = 0.6f, atk = 0.01f, rel = 0.001f, env[3] {}, gdb[3] {}, trim[3] { 1, 1, 1 };
    Split3 split;
};

// ---------- Dance master: three band compressor, drive, soft clipper, mix ----------
class DanceMaster
{
public:
    void prepare (double s) { sr = s; split.prepare (sr, 120.0, 2800.0); for (auto& e : env) e = -80.0f; for (auto& g : gdb) g = 0.0f;
        atk = 1.0f - std::exp (-1.0f / (0.008f * (float) sr)); rel = 1.0f - std::exp (-1.0f / (0.12f * (float) sr)); }
    void set (bool on_, float mix_, float drive_, float clip_, float lo, float mid, float hi)
    { on = on_; mix = clampf2 (mix_, 0.0f, 1.0f); drive = clampf2 (drive_, 0.0f, 1.0f); clip = clampf2 (clip_, 0.0f, 1.0f); amt[0] = lo; amt[1] = mid; amt[2] = hi; }
    void process (float& l, float& r)
    {
        if (! on || mix < 0.001f) { float b[3][2]; split.split (l, r, b); return; }
        float b[3][2]; split.split (l, r, b);
        float wl = 0.0f, wr = 0.0f;
        for (int k = 0; k < 3; ++k)
        {
            float g = 1.0f;
            if (amt[k] > 0.001f)
            {
                const float lev = linToDb (std::max (std::fabs (b[k][0]), std::fabs (b[k][1])));
                env[k] += (lev > env[k] ? atk : rel) * (lev - env[k]);
                const float thr = -14.0f - 18.0f * amt[k], ratio = 1.0f + 6.0f * amt[k];
                float red = env[k] > thr ? (env[k] - thr) * (1.0f - 1.0f / ratio) : 0.0f;
                gdb[k] += 0.01f * (-red - gdb[k]);
                g = dbToLin (gdb[k] + 0.3f * (14.0f + 18.0f * amt[k]) * (1.0f - 1.0f / ratio));       // makeup
            }
            wl += b[k][0] * g; wr += b[k][1] * g;
        }
        wl = shape (wl); wr = shape (wr);
        l += (wl - l) * mix; r += (wr - r) * mix;
    }
private:
    float shape (float x) const
    {
        if (drive > 0.001f) { const float k = 1.0f + 4.0f * drive; x = std::tanh (k * x) / k * (1.0f + 0.35f * (k - 1.0f)); }
        if (clip > 0.001f) { const float th = 1.0f - 0.55f * clip; x = th * std::tanh (x / th); }
        return x;
    }
    double sr = 48000.0; bool on = false; float mix = 0.5f, drive = 0.3f, clip = 0.3f, amt[3] { 0.0f, 0.5f, 0.4f }, atk = 0.01f, rel = 0.001f, env[3] {}, gdb[3] {};
    Split3 split;
};

// ---------- stereo tool: width (overall, low, high), mono below, mid and side level and drive ----------
class StereoTool
{
public:
    void prepare (double s) { sr = s; for (auto& f : lpW) f.reset(); for (auto& f : hpW) f.reset(); for (auto& f : lpM) f.reset(); lastMono = -1.0f; setSplit(); }
    void set (bool on_, float w, float wLo, float wHi, float monoHz, float midDb, float sideDb, float midDrv, float sideDrv)
    {
        on = on_; width = w; widthLo = wLo; widthHi = wHi; midG = dbToLin (midDb); sideG = dbToLin (sideDb); midDrive = midDrv; sideDrive = sideDrv;
        monoOn = monoHz > 20.5f;
        if (monoHz != lastMono) { lastMono = monoHz; for (auto& f : lpM) { f.highpass (sr, std::min (std::max (monoHz, 20.0f), 500.0f)); } }
    }
    void process (float& l, float& r)
    {
        if (! on) return;
        float m = 0.5f * (l + r), sd = 0.5f * (l - r);
        const bool widths = width != 1.0f || widthLo != 1.0f || widthHi != 1.0f;
        if (widths || monoOn)
        {
            float sh = sd;
            if (widths) { const float lo = lpW[0].process (sd), hi = hpW[0].process (sd); sh = width * (widthLo * lo + widthHi * hi); }
            if (monoOn) sh = lpM[0].process (sh);       // high pass: everything under the split frequency is removed from the side signal
            sd = sh;
        }
        m *= midG; sd *= sideG;
        if (midDrive > 0.001f) m = sat (m, midDrive);
        if (sideDrive > 0.001f) sd = sat (sd, sideDrive);
        l = m + sd; r = m - sd;
    }
private:
    static float sat (float x, float d) { const float k = 1.0f + 4.0f * d; return std::tanh (k * x) / k * (1.0f + 0.35f * (k - 1.0f)); }
    void setSplit() { for (auto& f : lpW) f.lowpass (sr, 250.0); for (auto& f : hpW) f.highpass (sr, 250.0); }
    double sr = 48000.0; bool on = false, monoOn = false; float width = 1, widthLo = 1, widthHi = 1, midG = 1, sideG = 1, midDrive = 0, sideDrive = 0, lastMono = -1.0f;
    LR4 lpW[1], hpW[1], lpM[1];
};

} // namespace purrone
