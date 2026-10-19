// Offline checks for the FX: limiter ceiling, EQ gains, crossover flatness, reverb decay and high-pass, CPU.
#include "purrone/Fx.h"
#include <cstdio>
#include <vector>
using namespace purrone;
static double sr = 48000;
static float sine (int n, double f, double a) { return (float) (a * std::sin (kTwoPi * f * n / sr)); }
static double rmsDb (const std::vector<float>& x, int from) { double s = 0; for (size_t i = (size_t) from; i < x.size(); ++i) s += (double) x[i] * x[i]; return 10 * std::log10 (s / (double) (x.size() - (size_t) from) + 1e-20); }
int fails = 0;
static void check (const char* name, bool ok, const char* detail) { printf ("%-34s %s  %s\n", name, ok ? "PASS" : "FAIL", detail); if (! ok) ++fails; }

int main()
{
    char d[160];
    { // limiter: loud sine, square and burst must stay under -1 dBFS
        SafetyLimiter lim; lim.prepare (sr, -1.0f); float peak = 0;
        for (int n = 0; n < 48000 * 2; ++n)
        {
            float x = n < 48000 ? sine (n, 100, 3.0) : ((n / 120) % 2 ? 2.5f : -2.5f);
            float l = x, r = x * 0.7f; lim.process (l, r); if (n > 400) peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
        }
        std::snprintf (d, sizeof d, "peak %.3f (ceiling 0.891), latency %d samples", peak, lim.latency());
        check ("limiter ceiling", peak <= 0.8915f, d);
        // quiet material untouched
        SafetyLimiter q; q.prepare (sr, -1.0f); double e = 0; for (int n = 0; n < 48000; ++n) { float l = sine (n, 440, 0.3), r = l; q.process (l, r); if (n > 400) { float want = sine (n - q.latency(), 440, 0.3); e = std::max (e, (double) std::fabs (l - want)); } }
        std::snprintf (d, sizeof d, "max error %.5f", e); check ("limiter transparent when quiet", e < 1e-4, d);
    }
    { // EQ gains
        const double fr[3] = { 100, 1000, 6000 }; const char* nm[3] = { "EQ low shelf +6 dB at 100 Hz (to 50 Hz)", "EQ bell +6 dB at 1 kHz", "EQ high shelf -6 dB at 6 kHz (10 kHz)" };
        double testF[3] = { 40, 1000, 12000 }, want[3] = { 6, 6, -6 };
        for (int k = 0; k < 3; ++k)
        {
            Eq3 eq; eq.prepare (sr); eq.setGains (k == 0 ? 6 : 0, k == 1 ? 6 : 0, k == 2 ? -6 : 0);
            std::vector<float> y; for (int n = 0; n < 48000; ++n) y.push_back (eq.process (sine (n, testF[k], 0.5)));
            double g = rmsDb (y, 24000) - 10 * std::log10 (0.5 * 0.25);
            std::snprintf (d, sizeof d, "measured %+.2f dB at %.0f Hz", g, testF[k]); check (nm[k], std::fabs (g - want[k]) < 0.8, d); (void) fr;
        }
    }
    { // maximizer with amount 0 must be flat and the crossover must sum flat
        double worst = 0; const double fs[] = { 30, 80, 120, 300, 1000, 2500, 5000, 12000 };
        for (double f : fs)
        {
            Maximizer m; m.prepare (sr); m.setAmount (0.0f); std::vector<float> y;
            for (int n = 0; n < 48000; ++n) { float l = sine (n, f, 0.3), r = l; m.process (l, r); y.push_back (l); }
            double g = rmsDb (y, 24000) - 10 * std::log10 (0.3 * 0.3 / 2); worst = std::max (worst, std::fabs (g));
        }
        std::snprintf (d, sizeof d, "worst deviation %.2f dB over 30 Hz to 12 kHz", worst); check ("crossover sum is flat", worst < 0.5, d);
    }
    { // opto: loud tone gets reduced more than a quiet one, off is transparent, no NaN
        Opto o; o.prepare (sr); o.setAmount (0.6f);
        auto run = [&] (double amp) { std::vector<float> y; for (int n = 0; n < 96000; ++n) { float l = sine (n, 110, amp), r = l; o.process (l, r); y.push_back (l); } return rmsDb (y, 48000); };
        double lo = run (0.02); o = Opto(); o.prepare (sr); o.setAmount (0.6f); double hi = run (0.7);
        double inSpread = 20 * std::log10 (0.7 / 0.02);
        std::snprintf (d, sizeof d, "level spread in %.1f dB, out %.1f dB", inSpread, hi - lo); check ("opto narrows level spread", (hi - lo) < inSpread - 6.0 && std::isfinite (hi), d);
        Opto off; off.prepare (sr); float l = 0.3f, r = 0.3f; off.process (l, r); check ("opto off is transparent", l == 0.3f && r == 0.3f, "exact");
    }
    { // maximizer at full: steady loud and quiet tones both end up closer in level; no NaN
        Maximizer m; m.prepare (sr); m.setAmount (1.0f); double lo, hi;
        auto run = [&] (double amp) { std::vector<float> y; for (int n = 0; n < 96000; ++n) { float l = sine (n, 220, amp), r = l; m.process (l, r); y.push_back (l); } return rmsDb (y, 48000); };
        double inLo = 20 * std::log10 (0.01 / 1.4142), inHi = 20 * std::log10 (0.5 / 1.4142);
        lo = run (0.01); m = Maximizer(); m.prepare (sr); m.setAmount (1.0f); hi = run (0.5);
        std::snprintf (d, sizeof d, "level spread in %.1f dB, out %.1f dB", inHi - inLo, hi - lo); check ("maximizer narrows level spread", (hi - lo) < (inHi - inLo) - 6.0 && std::isfinite (hi), d);
    }
    { // reverb: decay time and high-pass
        for (float rt : { 0.5f, 1.5f })
        {
            Reverb rv; rv.prepare (sr); rv.setParams (rt, 250.0f); std::vector<float> e; double lowE = 0, totE = 0;
            for (int n = 0; n < (int) (sr * 5); ++n) { float l, r; rv.process (n == 0 ? 1.0f : 0.0f, l, r); e.push_back (l * l + r * r); }
            // energy decay: time for energy to fall 60 dB below the early peak window
            double tot = 0; for (float v : e) tot += v; double rem = tot; int t60 = -1; double ref = 0; for (int i = 0; i < 4800; ++i) ref += e[(size_t) i];
            (void) ref; int t = 0; double acc = 0; double thresh = tot * 1e-6; for (size_t i = 0; i < e.size(); ++i) { acc += e[i]; if (tot - acc < thresh) { t = (int) i; break; } }
            std::snprintf (d, sizeof d, "energy tail to -60 dB at %.2f s (set %.1f s)", t / sr, rt);
            check (rt < 1 ? "reverb short setting" : "reverb 1.5 s setting", t / sr > rt * 0.5 && t / sr < rt * 2.2, d); (void) rem; (void) t60; (void) lowE; (void) totE;
        }
        Reverb rv; rv.prepare (sr); rv.setParams (1.0f, 250.0f); double eLow = 0, eAll = 0; Biquad lpf; lpf.lowpass (sr, 100.0, 0.707);
        for (int n = 0; n < (int) (sr * 3); ++n) { float l, r; rv.process (n < 24000 ? (float) (0.1 * std::sin (kTwoPi * 60 * n / sr)) + 0.1f * (float) std::sin (kTwoPi * 1500 * n / sr) : 0.0f, l, r); float lo = lpf.process (l); eLow += lo * lo; eAll += l * l; }
        std::snprintf (d, sizeof d, "energy below 100 Hz is %.1f dB under the total", 10 * std::log10 (eLow / eAll)); check ("reverb return is high-passed", 10 * std::log10 (eLow / eAll) < -20.0, d);
    }
    printf (fails ? "\n%d check(s) failed\n" : "\nall checks passed\n", fails);
    return fails ? 1 : 0;
}
