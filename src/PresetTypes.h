// Factory preset data types.
#pragma once
#include <utility>
#include <vector>

namespace purrone
{
// values: parameter id and real (not normalised) value; anything not listed keeps its default.
// seq: optional step sequence for pattern A, space separated steps: "-" rest, or a note like "C2", "F#3", "Bb2" (C2 lowest, C5 highest), with
// "a" for accent and "s" for slide added, e.g. "E2as". The number of steps becomes the sequence length.
struct FactoryPreset { const char* name; const char* category; std::vector<std::pair<const char*, float>> values; const char* seq = ""; };
inline constexpr int kFactoryBanks = 4;    // A to D are factory, E to H are the user's
inline constexpr int kBankCount = 8;
inline constexpr int kBankSize = 128;     // one bank = 128 sounds (MIDI program change plus bank select, SysEx dumps)
} // namespace purrone
