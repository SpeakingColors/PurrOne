# Roadmap

- [x] Header-only mono engine: PolyBLEP oscillators with hard sync, sub, 2x oversampled, ZDF 4 pole filter, ADSR x2, LFO, mod matrix, glide, keyboard modes
- [x] Offline render and analysis tools
- [ ] Alias reduction pass (4 point PolyBLEP or 4x oversampling), measure again
- [ ] Filter modes (LP/HP), keyboard tracking check, level compensation
- [ ] Arpeggiator and 2 x 64 step sequencer
- [x] JUCE plugin shell (VST3 + standalone), parameters, state (builds; untested in a host)
- [x] FX (offline tests pass): 3 band EQ, short reverb with high-pass, drive/saturation/fuzz, multiband maximizer, safety limiter
- [x] Preset manager and 11 factory presets (level matching to check by ear)
- [x] Interface mockup (horizontal, classic mono layout)
- [ ] JUCE editor from the mockup (dark, white text, wood cheeks, LED colours, output meter)
- [ ] Offline installers (Windows exe, Flatpak, macOS pkg), PDF manual
- [ ] Test in a real host (Bitwig, Reaper); pluginval level 5 in CI
