// Offline render harness: writes raw float32 mono files for analysis.
#include "purrone/Engine.h"
#include <chrono>
#include <cstdio>
#include <vector>
#include <string>
using namespace purrone;

static void renderNote (const char* name, Params p, int note, float holdSec, float tailSec, double sr = 48000.0)
{
    Engine e; e.prepare (sr); e.setParams (p); e.noteOn (note, 1.0f);
    int nHold = (int) (holdSec * sr), nTail = (int) (tailSec * sr);
    std::vector<float> buf ((size_t) (nHold + nTail));
    for (int i = 0; i < nHold; i += 64) e.process (buf.data() + i, std::min (64, nHold - i));
    e.noteOff (note);
    for (int i = 0; i < nTail; i += 64) e.process (buf.data() + nHold + i, std::min (64, nTail - i));
    FILE* f = fopen (name, "wb"); fwrite (buf.data(), 4, buf.size(), f); fclose (f);
}

int main (int argc, char** argv)
{
    std::string dir = argc > 1 ? argv[1] : ".";
    auto path = [&] (const char* n) { return dir + "/" + n; };
    Params p;

    // 1 aliasing: wide open saw, high note
    p = Params(); p.cutoffHz = 20000; p.kbdAmt = 0; p.reso = 0; p.mixA = 1; p.aSaw = true; p.aS = 1;
    renderNote (path ("saw_c7.raw").c_str(), p, 96, 2.0f, 0.1f);
    renderNote (path ("saw_c5.raw").c_str(), p, 72, 2.0f, 0.1f);
    // 2 pulse
    p.aSaw = false; p.aPulse = true; p.aPW = 0.3f;
    renderNote (path ("pulse_c6.raw").c_str(), p, 84, 2.0f, 0.1f);
    // 3 sync
    p = Params(); p.cutoffHz = 20000; p.kbdAmt = 0; p.reso = 0; p.mixA = 1; p.mixB = 0; p.aSync = true; p.aS = 1; p.bTune = 7.0f; p.bOct = 2;
    renderNote (path ("sync_c4.raw").c_str(), p, 60, 2.0f, 0.1f);
    // 4 filter self oscillation
    p = Params(); p.mixA = 0; p.mixNoise = 0.0f; p.cutoffHz = 880; p.reso = 1.0f; p.kbdAmt = 0; p.aS = 1;
    renderNote (path ("selfosc.raw").c_str(), p, 60, 2.0f, 0.1f);
    // 5 sub bass, 6 reso sweep stability at extremes
    p = Params(); p.mixA = 0.7f; p.mixSub = 0.9f; p.cutoffHz = 400; p.reso = 0.3f; p.aS = 1; p.kbdAmt = 0.3f;
    renderNote (path ("sub_c1.raw").c_str(), p, 36, 1.0f, 0.3f);
    p = Params(); p.mixA = 1; p.mixB = 1; p.mixNoise = 1; p.mixSub = 1; p.cutoffHz = 20; p.reso = 1.0f; p.fenvAmt = 1.0f; p.xstr = 1.0f; p.aS = 1; p.fS = 1;
    renderNote (path ("extreme.raw").c_str(), p, 60, 1.0f, 0.3f);

    // CPU: 10 s of audio, default patch with sync + mod
    p = Params(); p.aSaw = p.aPulse = true; p.aSync = true; p.bSaw = true; p.modAmt[2] = 0.5f; p.modRoute[2] = 1; p.modDest[4] = 1;
    Engine e; e.prepare (48000.0); e.setParams (p); e.noteOn (48, 1.0f);
    std::vector<float> b (64); auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 48000 * 10 / 64; ++i) e.process (b.data(), 64);
    double s = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    printf ("cpu: %.2f%% of one core at 48k (10 s audio in %.3f s)\n", s / 10.0 * 100.0, s);
    return 0;
}
