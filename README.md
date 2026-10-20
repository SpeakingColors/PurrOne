# PurrOne

A free, open source mono synth plugin with a thick, smooth sound, built for bass and lead. Linux first (VST3), also targeting Windows and macOS.

Status: early development (test builds). The sound engine, effects, factory presets and a VST3/standalone shell (generic editor) build. The custom interface, step sequencer, arpeggiator and new FX pages are in. Not yet tested in a real host. See [ROADMAP.md](ROADMAP.md) and [docs/SPEC.md](docs/SPEC.md).

## What it is
- Two oscillators with hard sync, plus a warm sub oscillator, noise and a mixer.
- A 4 pole self-oscillating filter with low-pass, high-pass and band-pass modes, two envelopes, an LFO and a mod matrix with a mod wheel path.
- Glide and keyboard modes (normal, retrig, repeat, drone). Step sequencer (up to 64 steps, 4 patterns, swing, slide, accent, optional 303 squelch) and arpeggiator, started by the host, a key or a chosen note, with optional MIDI out (off by default).
- Built-in effects, all with sane defaults that stay clean and never harsh (built, offline tested): 5 band graphic EQ (stereo, mid or side per band, optional dynamic, boost or cut, with live display), short reverb with pre-delay, damping, width and ducking, saturation and drive, bit crusher, opto, OTT, Dance master, stereo width, multiband maximizer, safety limiter. New modules start off or neutral, so the sound is unchanged until you use them.
- Preset manager with 384 factory sounds in banks A to C (128 each, not yet auditioned by ear), four empty banks D to G for your own sounds, and a preset dropdown on each FX module.
- Dark interface, white text, wood side panels, coloured lights only on small LEDs, big stereo output meter. Four pages: Sound, Seq / Arp, FX (with analyzer) and Settings (banks, tuning, random LFO, quality, window). The panel mockup is in docs/mockup.

## Install and remove
Each release is one zip with an installer per system (Windows .exe, macOS .pkg, Linux Flatpak and plain .vst3), the PDF manual, INSTALL.txt and SHA256SUMS. Everything works offline. The Windows and macOS uninstallers remove everything they installed and keep your saved presets unless you choose to delete them. See [packaging/INSTALL.txt](packaging/INSTALL.txt).

## Build the plugin
Needs JUCE 8 (local copy or FetchContent), CMake and a C++17 compiler.
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Outputs a VST3 and a standalone app.

## Build the offline test tools
```
g++ -O2 -std=c++17 -Iinclude tools/render.cpp -o render
./render /tmp/out
python3 tools/analyze.py /tmp/out
g++ -O2 -std=c++17 -Iinclude tools/fxtest.cpp -o fxtest && ./fxtest
```

## Licence
AGPL-3.0-or-later, see [LICENSE](LICENSE). Free to use, no account, no network, works offline.

An original design, inspired by classic two oscillator analog mono synths. Not affiliated with or endorsed by any synth maker. All trademarks belong to their owners.

## Support

<p align="center">
  <a href="https://ko-fi.com/studio87"><img src="https://img.shields.io/badge/Support_Studio_87-on_Ko--fi-FF5E5B?style=for-the-badge&logo=kofi&logoColor=white" alt="Support Studio 87 on Ko-fi"></a>
</p>

☕ Free and open source, made by one person. If it helps your music, a coffee on [Ko-fi](https://ko-fi.com/studio87) keeps it coming. 💜
