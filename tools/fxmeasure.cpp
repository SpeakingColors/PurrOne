// Offline check: how much does each FX change a typical synth signal? (rms change and difference to dry)
#include "purrone/Fx.h"
#include "purrone/Fx2.h"
#include <cstdio>
using namespace purrone;
static std::vector<float> sig (double sr, float peak)
{
    std::vector<float> s ((size_t) (sr * 4)); double ph = 0, ph2 = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        double t = i / sr, nt = std::fmod (t, 0.5); double f = (((int) (t / 0.5)) % 2) ? 82.4 : 110.0;
        ph += f / sr; ph -= std::floor (ph); ph2 += f * 0.5 / sr; ph2 -= std::floor (ph2);
        double env = std::exp (-nt * 5.0);
        double saw = 2 * ph - 1, sub = std::sin (kTwoPi * ph2);
        s[i] = (float) (peak * env * (0.6 * saw + 0.4 * sub));
    }
    return s;
}
static void rep (const char* n, const std::vector<float>& d, const std::vector<float>& w)
{
    double sd = 0, sw = 0, sx = 0, pd = 0, pw = 0;
    for (size_t i = 0; i < d.size(); ++i) { sd += d[i] * d[i]; sw += w[i] * w[i]; sx += (w[i] - d[i]) * (w[i] - d[i]); pd = std::max (pd, (double) std::fabs (d[i])); pw = std::max (pw, (double) std::fabs (w[i])); }
    auto db = [] (double x) { return 10 * std::log10 (std::max (x, 1e-12)); };
    std::printf ("%-34s rmsChange %+6.2f dB   diff-vs-dry %6.1f dB   peak %+5.1f -> %+5.1f dBFS\n", n, db (sw / sd), db (sx / sd), 20 * std::log10 (pd), 20 * std::log10 (pw));
}
template <class F> static void run (const char* n, float peak, F f)
{
    const double sr = 48000; auto d = sig (sr, peak); std::vector<float> w = d;
    f (w, sr); rep (n, d, w);
}
static void eqTest (float dyn, float amp, float staticG)
{
    const double sr = 48000; Eq5 e; e.prepare (sr); e.setBand (2, true, 1000.0f, staticG, 0.8f, dyn, -30.0f, 0);
    double a = 0, b = 0; const int n = (int) sr * 2;
    for (int i = 0; i < n; ++i) { const float x = amp * (float) std::sin (kTwoPi * 1000.0 * i / sr), y = e.processMono (x); if (i > n / 2) { a += x * x; b += y * y; } }
    std::printf ("dynamic EQ band 1 kHz, thr -30, dyn %+.0f, static %+.0f, input %.0f dBFS peak: gain %+.2f dB (display says %+.2f)\n", dyn, staticG, 20 * std::log10 (amp), 10 * std::log10 (b / a), e.liveDynDb (2));
}
int main()
{
    eqTest (-9, 0.25f, 0); eqTest (9, 0.25f, 0); eqTest (-9, 0.005f, 0); eqTest (6, 0.25f, -3); eqTest (-6, 0.25f, 3);
    for (float peak : { 0.5f, 0.15f })
    {
        std::printf ("--- synth level peak %.2f (%.1f dBFS)\n", peak, 20 * std::log10 (peak));
        run ("Crush on, new defaults 8bit/12k", peak, [] (auto& w, double sr) { Crush c; c.prepare (sr); c.set (true, 8, 12000, 0.7f); for (auto& x : w) x = c.process (x); });
        run ("Crush on, 8bit/24k mix .5", peak, [] (auto& w, double sr) { Crush c; c.prepare (sr); c.set (true, 8, 24000, 0.5f); for (auto& x : w) x = c.process (x); });
        for (float a : { 0.3f, 0.5f, 1.0f })
        {
            char n[64]; std::snprintf (n, 64, "Opto amount %.1f", a);
            run (n, peak, [a] (auto& w, double sr) { Opto o; o.prepare (sr); o.setAmount (a); o.setSpeed (1); for (auto& x : w) { float l = x, r = x; o.process (l, r); x = l; } });
        }
        run ("OTT default (.5 amt, .6 depth)", peak, [] (auto& w, double sr) { Ott o; o.prepare (sr); o.set (true, 0.5f, 0.6f, 0.4f, 0, 0, 0); for (auto& x : w) { float l = x, r = x; o.process (l, r); x = l; } });
        run ("OTT amt 1 depth 1", peak, [] (auto& w, double sr) { Ott o; o.prepare (sr); o.set (true, 1.0f, 1.0f, 0.4f, 0, 0, 0); for (auto& x : w) { float l = x, r = x; o.process (l, r); x = l; } });
        run ("Dance default", peak, [] (auto& w, double sr) { DanceMaster o; o.prepare (sr); o.set (true, 0.8f, 0.4f, 0.4f, 0.5f, 0.6f, 0.5f); for (auto& x : w) { float l = x, r = x; o.process (l, r); x = l; } });
        run ("Dance full (mix1 all .8)", peak, [] (auto& w, double sr) { DanceMaster o; o.prepare (sr); o.set (true, 1.0f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f); for (auto& x : w) { float l = x, r = x; o.process (l, r); x = l; } });
    }
}
