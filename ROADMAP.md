# Roadmap

- [x] Header-only mono engine: PolyBLEP oscillators with hard sync, sub, 2x oversampled, ZDF 4 pole filter, ADSR x2, LFO, mod matrix, glide, keyboard modes
- [x] Offline render and analysis tools
- [ ] Alias reduction pass (4 point PolyBLEP or 4x oversampling), measure again
- [ ] Filter modes (LP/HP), keyboard tracking check, level compensation
- [x] Arpeggiator and step sequencer (4 patterns x 64 steps, host / key / note start, MIDI out)
- [x] JUCE plugin shell (VST3 + standalone), parameters, state (builds; untested in a host)
- [x] FX (offline tests pass): 3 band EQ, short reverb with high-pass, drive (analog clip), transistor (Neve-style), fuzz, multiband maximizer, safety limiter
- [x] Preset manager and 384 factory sounds in banks A to C (level matching and auditioning by ear still to do)
- [x] Interface mockup (horizontal, classic mono layout)
- [x] JUCE editor from the mockup: Sound, Seq / Arp, FX and Settings pages
- [x] Offline installers with uninstallers that keep your presets by default (Windows exe, macOS pkg plus Uninstall app, Flatpak, plain .vst3), PDF manual (release workflow; test builds are published as pre-releases, latest v0.3.0)
- [x] pluginval in CI (strictness 10)
- [ ] Test in a real host (Bitwig, Reaper)
