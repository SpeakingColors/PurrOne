# PurrOne

A free, open source mono synth plugin with a thick, smooth sound, built for bass and lead. Linux first (VST3), also targeting Windows and macOS.

Status: early development. The sound engine exists and is measured offline; the plugin shell, effects, presets and interface are next. See [ROADMAP.md](ROADMAP.md) and [docs/SPEC.md](docs/SPEC.md).

## What it is
- Two oscillators with hard sync, plus a warm sub oscillator, noise and a mixer.
- A 4 pole self-oscillating low-pass filter, two envelopes, an LFO and a mod matrix with a mod wheel path.
- Glide, arpeggiator and step sequencer (planned).
- Built-in effects, all with sane defaults that stay clean and never harsh (planned): 3 band EQ, short reverb with high-pass, saturation and drive, multiband maximizer, safety limiter.
- Preset manager with a factory bank of basses, subs and leads (planned).
- Dark interface, white text, wood side panels, coloured lights only on small LEDs, big stereo output meter (planned).

## Build the offline test tool
```
g++ -O2 -std=c++17 -Iinclude tools/render.cpp -o render
./render /tmp/out
python3 tools/analyze.py /tmp/out
```

## Licence
AGPL-3.0-or-later, see [LICENSE](LICENSE). Free to use, no account, no network, works offline.

An original design, inspired by classic two oscillator analog mono synths. Not affiliated with or endorsed by any synth maker. All trademarks belong to their owners.
