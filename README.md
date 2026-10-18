# PurrOne

A free, open source mono synth plugin with a thick, smooth sound, built for bass and lead. Linux first (VST3), also targeting Windows and macOS.

Status: early development. The sound engine, effects, factory presets and a VST3/standalone shell (generic editor) build. The custom interface, arpeggiator and sequencer are next. Not yet tested in a real host. See [ROADMAP.md](ROADMAP.md) and [docs/SPEC.md](docs/SPEC.md).

## What it is
- Two oscillators with hard sync, plus a warm sub oscillator, noise and a mixer.
- A 4 pole self-oscillating low-pass filter, two envelopes, an LFO and a mod matrix with a mod wheel path.
- Glide and keyboard modes (normal, retrig, repeat, drone). Arpeggiator and step sequencer (planned).
- Built-in effects, all with sane defaults that stay clean and never harsh (built, offline tested): 3 band EQ, short reverb with high-pass, saturation and drive, multiband maximizer, safety limiter.
- Preset manager with 11 factory presets of basses, subs and leads.
- Dark interface, white text, wood side panels, coloured lights only on small LEDs, big stereo output meter. A horizontal, classic-mono-synth style panel mockup is in docs/mockup (editor not built yet).

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
