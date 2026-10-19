# PurrOne spec (v0)

## Voice (mono)
- Osc A: saw, pulse (width), hard sync from B, octave 16' to 2', tune +-6 st. Always follows the keyboard.
- Osc B: saw, triangle, pulse, low frequency mode, keyboard tracking switch.
- Sub: square one octave under A, rounded. Our addition for thick low end.
- Mixer: A, B, sub, noise.
- Filter: 4 pole, zero delay feedback, soft saturation in the loop, self-oscillation at full resonance. Modes: low pass, high pass, band pass.
- Envelopes: two ADSR, 2 ms to 6 s.
- LFO: saw, triangle, pulse, 0.1 to 30 Hz.
- Mod matrix: sources filter envelope, osc B, LFO; each with amount and direct/wheel/off route; destinations osc A freq, osc A PW, osc B freq, osc B PW, filter.
- Keyboard: normal (low note priority, single trigger) or retrigger (last note); repeat; drone; glide normal or auto.
- Engine runs at 2x internal rate with a linear phase decimator.

## Sound rules
- Smooth, never harsh. Check aliasing, resonance peaks, hard clipping. Slow tiny drift is allowed.
- Clean and dirty basses both possible. FX defaults are conservative.

## FX chain (implemented order)
Drive/saturation/fuzz, 3 band EQ, short reverb (high-passed return, length capped), multiband maximizer, safety limiter.

## Interface
Horizontal panel in the style of classic mono synths: modulation block on the left, oscillators, mixer, filter and envelopes across, volume and meter on the right, FX and keys below (docs/mockup). Dark background, white text, thin wood side cheeks, colour only on small LEDs, large stereo output meter with dB scale, preset manager, hover info bubbles with off switch.

## Measured so far (offline, 48 kHz)
See tools/analyze.py. Numbers are posted in the commit that changes them.
