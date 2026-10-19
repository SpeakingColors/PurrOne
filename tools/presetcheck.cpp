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
