// GENERATED from docs/mockup/purrone-mockup.html (Seq / Arp, FX and Settings pages). Do not edit by hand. Same 1700 x 828 space as Layout.h.
#pragma once
#include <iterator>
#include "Layout.h"
namespace purrone::layout2
{
using layout::R; using layout::Sec; using layout::Knob; using layout::Tog; using layout::Btn; using layout::Seg;
struct Item { const char* kind; const char* text; const char* sub; R r; R r2; };

inline constexpr Btn segBtns2[] = {
  { "OFF", { 62,151,45,24 } },
  { "SEQ", { 107,151,46,24 } },
  { "ARP", { 153,151,46,24 } },
  { "1/8", { 207,151,38,24 } },
  { "1/8T", { 245,151,46,24 } },
  { "1/16", { 291,151,45,24 } },
  { "1/16T", { 336,151,54,24 } },
  { "1/32", { 389,151,45,24 } },
  { "HOST", { 443,151,55,24 } },
  { "FREE", { 498,151,54,24 } },
  { "A", { 132,236,28,24 } },
  { "B", { 160,236,28,24 } },
  { "C", { 188,236,28,24 } },
  { "D", { 215,236,28,24 } },
  { "1", { 295,236,26,24 } },
  { "2", { 321,236,26,24 } },
  { "3", { 346,236,26,24 } },
  { "4", { 372,236,26,24 } },
  { "1", { 458,236,26,24 } },
  { "2", { 484,236,26,24 } },
  { "4", { 509,236,26,24 } },
  { "8", { 535,236,26,24 } },
  { "12", { 561,236,33,24 } },
  { "16", { 594,236,33,24 } },
  { "24", { 627,236,33,24 } },
  { "32", { 661,236,33,24 } },
  { "48", { 694,236,33,24 } },
  { "64", { 727,236,33,24 } },
  { "UP", { 62,534,37,24 } },
  { "DOWN", { 99,534,60,24 } },
  { "UP/DN", { 158,534,60,24 } },
  { "RANDOM", { 219,534,78,24 } },
  { "PLAYED", { 296,534,71,24 } },
  { "1", { 376,534,26,24 } },
  { "2", { 401,534,26,24 } },
  { "3", { 427,534,26,24 } },
  { "4", { 453,534,26,24 } },
  { "PRE", { 242,105,46,24 } },
  { "POST", { 288,105,55,24 } },
  { "BOTH", { 342,105,56,24 } },
  { "SPECTRUM", { 1332,105,93,24 } },
  { "SCOPE", { 1425,105,65,24 } },
  { "PHASE", { 1490,105,64,24 } },
  { "1", { 287,105,26,24 } },
  { "2", { 313,105,26,24 } },
  { "3", { 338,105,26,24 } },
  { "4", { 364,105,26,24 } },
  { "5", { 390,105,26,24 } },
  { "STEREO", { 242,483,73,24 } },
  { "MID", { 315,483,43,24 } },
  { "SIDE", { 358,483,50,24 } },
  { "SPECTRUM", { 1332,105,93,24 } },
  { "SCOPE", { 1425,105,65,24 } },
  { "PHASE", { 1490,105,64,24 } },
  { "SPECTRUM", { 1332,105,93,24 } },
  { "SCOPE", { 1425,105,65,24 } },
  { "PHASE", { 1490,105,64,24 } },
  { "SLOW", { 242,105,58,24 } },
  { "MED", { 300,105,48,24 } },
  { "FAST", { 347,105,53,24 } },
  { "SPECTRUM", { 1332,105,93,24 } },
  { "SCOPE", { 1425,105,65,24 } },
  { "PHASE", { 1490,105,64,24 } },
  { "SPECTRUM", { 1332,105,93,24 } },
  { "SCOPE", { 1425,105,65,24 } },
  { "PHASE", { 1490,105,64,24 } },
  { "EQUAL", { 555,105,65,24 } },
  { "JUST", { 620,105,53,24 } },
  { "PYTHAGOREAN", { 672,105,122,24 } },
  { "CUSTOM", { 794,105,76,24 } },
  { "432", { 878,105,41,24 } },
  { "440", { 918,105,41,24 } },
  { "442", { 959,105,41,24 } },
  { "NORMAL 4X", { 62,471,97,24 } },
  { "HIGH 8X", { 159,471,73,24 } },
  { "2", { 555,471,26,24 } },
  { "5", { 580,471,26,24 } },
  { "12", { 606,471,33,24 } },
  { "24", { 639,471,33,24 } },
  { "CC1", { 680,471,45,24 } },
  { "CC74", { 725,471,53,24 } },
  { "70%", { 1328,471,45,24 } },
  { "85%", { 1373,471,45,24 } },
  { "100%", { 1417,471,53,24 } },
  { "115%", { 1470,471,52,24 } },
  { "130%", { 1522,471,53,24 } },
};
inline constexpr Sec seq_secs[] = {
  { "sqTrans", "TRANSPORT", "HOST SYNCED, OR FREE RUNNING", { 50,71,1600,118 } },
  { "sqSeq", "SEQUENCE", "CLICK A NOTE TO RAISE IT, SHIFT+CLICK LOWERS; CHANCE AND RATCHET CYCLE ON CLICK (SHIFT = BACK)", { 50,199,1600,291 } },
  { "sqArp", "ARP", "WHEN MODE IS ARP", { 50,500,454,175 } },
  { "sqPre", "SEQUENCE PRESETS", "THEIR OWN FOLDER, LOAD INTO ANY SOUND", { 514,500,672,175 } },
  { "sqMidi", "BUNDLE AND MIDI OUT", "", { 1196,500,454,175 } },
};
inline constexpr Knob seq_knobs[] = {
  { "seqBpm", "TEMPO", { 949,104,62,74 }, { 958,118,44,44 }, 'N', "Free running tempo. With Sync on Host the DAW tempo is used instead." },
  { "seqSwing", "SWING", { 1019,104,62,74 }, { 1028,118,44,44 }, 'P', "Delays every second step. 50% is straight." },
  { "seqGate", "GATE LEN", { 1089,104,62,74 }, { 1098,118,44,44 }, 'P', "How long each note is held, as a share of the step." },
  { "seqAcc", "ACCENT", { 1159,104,62,74 }, { 1168,118,44,44 }, 'P', "How much louder and brighter an accented step is." },
  { "seqSlide", "SLIDE", { 1229,104,62,74 }, { 1238,118,44,44 }, 'Y', "Glide time on slide steps." },
  { "seqHuman", "HUMANISE", { 1299,104,62,74 }, { 1309,118,44,44 }, 'P', "Small random timing drift." },
  { "seqTrans", "TRANSPOSE", { 1369,104,62,74 }, { 1384,118,44,44 }, 'W', "Shifts the whole sequence in semitones." },
  { "cutoff", "CUTOFF", { 1439,104,62,74 }, { 1448,118,44,44 }, 'C', "The same filter cutoff as on the Sound page, here so you can sweep it while the sequence plays." },
  { "reso", "RESO", { 1509,104,62,74 }, { 1518,118,44,44 }, 'P', "The same resonance as on the Sound page." },
  { "midiCh", "CHANNEL", { 1207,567,62,74 }, { 1216,581,44,44 }, 'I', "" },
  { "midiVel", "VELOCITY", { 1277,567,62,74 }, { 1286,581,44,44 }, 'I', "Velocity of normal steps." },
  { "midiAcc", "ACCENT VEL", { 1347,567,62,74 }, { 1362,581,44,44 }, 'I', "Velocity of accented steps." },
};
inline constexpr Tog seq_togs[] = {
  { "arpLatch", "LATCH", { 61,565,81,26 }, 0xffff9000u, "Keeps the arp running after you let go of the keys." },
  { "seqBundle", "SAVE SEQUENCE INSIDE SOUNDS", { 1207,533,262,26 }, 0xff02ebbau, "On: saving a sound also stores the current sequence, and loading it brings the sequence back. Off: sounds and sequences stay separate. Sequence presets are always separate files too." },
  { "midiOut", "MIDI OUT", { 1475,533,98,26 }, 0xffff1383u, "Off by default. On: the sequence or arp also sends its notes out as MIDI, so it can play another synth." },
};
inline constexpr Seg seq_segs[] = {
  { "seqMode", { 61,150,139,26 }, 0, 3, false, "Off: PurrOne plays your keys as usual. Seq: the step sequence plays. Arp: the held keys are played one at a time." },
  { "seqRate", { 206,150,229,26 }, 3, 5, false, "Step length. T is triplet." },
  { "seqSync", { 442,150,111,26 }, 8, 2, false, "Host follows the DAW tempo and transport. Free runs on its own tempo. In this mockup the tempo knob is used for both." },
  { "seqPat", { 131,235,113,26 }, 10, 4, false, "" },
  { "seqPage", { 294,235,105,26 }, 14, 4, false, "" },
  { "seqStepsSel", { 457,235,304,26 }, 18, 10, false, "" },
  { "arpPat", { 61,533,308,26 }, 28, 5, false, "Order the held keys are played in." },
  { "arpOct", { 375,533,105,26 }, 33, 4, false, "How many octaves the arp climbs." },
};
inline constexpr Item seq_items[] = {
  { "lab", "Steps", "", { 559,156,47,14 }, { 0,0,0,0 } },
  { "lab", "MIDI out", "", { 837,156,66,14 }, { 0,0,0,0 } },
  { "lab", "Pattern", "", { 61,241,64,14 }, { 0,0,0,0 } },
  { "lab", "Page", "", { 250,241,38,14 }, { 0,0,0,0 } },
  { "lab", "Steps", "", { 405,241,47,14 }, { 0,0,0,0 } },
  { "list", "sqPre", "", { 525,533,650,92 }, { 0,0,0,0 } },
  { "stepper", "", "", { 611,148,122,30 }, { 642,149,56,28 } },
  { "led", "", "", { 915,158,10,10 }, { 0,0,0,0 } },
  { "lanes", "", "", { 61,271,1578,150 }, { 0,0,0,0 } },
  { "lanenum0", "1", "", { 143,271,90,22 }, { 0,0,0,0 } },
  { "lanenum1", "2", "", { 237,271,90,22 }, { 0,0,0,0 } },
  { "lanenum15", "16", "", { 1549,271,90,22 }, { 0,0,0,0 } },
  { "rowlabel", "Step", "", { 61,275,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Note", "", { 61,295,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Gate", "", { 61,316,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Accent", "", { 61,337,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Slide", "", { 61,358,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Chance", "", { 61,379,78,14 }, { 0,0,0,0 } },
  { "rowlabel", "Ratchet", "", { 61,400,78,14 }, { 0,0,0,0 } },
  { "cell0a", "", "", { 143,297,90,28 }, { 0,0,0,0 } },
  { "cell0b", "", "", { 237,297,90,28 }, { 0,0,0,0 } },
  { "cell1a", "", "", { 143,329,90,28 }, { 0,0,0,0 } },
  { "cell1b", "", "", { 237,329,90,28 }, { 0,0,0,0 } },
  { "cell2a", "", "", { 143,361,90,28 }, { 0,0,0,0 } },
  { "cell2b", "", "", { 237,361,90,28 }, { 0,0,0,0 } },
  { "cell3a", "", "", { 143,393,90,28 }, { 0,0,0,0 } },
  { "cell3b", "", "", { 237,393,90,28 }, { 0,0,0,0 } },
  { "btn", "Play", "play", { 739,147,92,31 }, { 0,0,0,0 } },
  { "btn", "Random", "", { 767,232,89,31 }, { 0,0,0,0 } },
  { "btn", "Clear", "", { 862,232,74,31 }, { 0,0,0,0 } },
  { "btn", "Copy", "", { 942,232,65,31 }, { 0,0,0,0 } },
  { "btn", "Paste", "", { 1013,232,72,31 }, { 0,0,0,0 } },
  { "btn", "Save as...", "", { 525,633,101,31 }, { 0,0,0,0 } },
  { "btn", "Delete", "", { 632,633,82,31 }, { 0,0,0,0 } },
  { "btn", "Share", "", { 720,633,75,31 }, { 0,0,0,0 } },
  { "btn", "Import", "", { 800,633,81,31 }, { 0,0,0,0 } },
};
inline constexpr Sec fxColour_secs[] = {
  { "fxCol", "COLOUR", "DRIVE, TRANSISTOR, FUZZ", { 230,71,535,152 } },
  { "fxCrush", "CRUSH", "BIT AND RATE REDUCER", { 775,71,535,152 } },
  { "", "TRANSFER CURVE", "WHAT THE THREE COLOUR STAGES DO TO A SINE, IN TO OUT", { 230,233,1080,442 } },
};
inline constexpr Knob fxColour_knobs[] = {
  { "drive", "DRIVE", { 241,138,62,74 }, { 250,152,44,44 }, 'P', "Analog clip: a symmetric soft clip, the classic overdriven-opamp sound. Mostly odd harmonics. Past 80% it gets nasty, still smooth." },
  { "xstr", "TRANSISTOR", { 311,138,62,74 }, { 328,152,44,44 }, 'P', "Neve-style transistor stage: asymmetric soft clip, warm even harmonics and some extra low-end weight. Low is warm; high gets thick. Past 80% it gets nasty, still smooth." },
  { "fuzz", "FUZZ", { 381,138,62,74 }, { 390,152,44,44 }, 'P', "Harder, lopsided fuzz. Past 80% it adds an octave-up growl, still smooth." },
  { "crushBits", "BITS", { 786,138,62,74 }, { 795,152,44,44 }, 'R', "Lower bit depth, grittier." },
  { "crushRate", "RATE", { 856,138,62,74 }, { 865,152,44,44 }, 'C', "Sample rate reducer." },
  { "crushMix", "MIX", { 926,138,62,74 }, { 935,152,44,44 }, 'P', "" },
};
inline constexpr Tog fxColour_togs[] = {
  { "crushOn", "ON", { 786,104,56,26 }, 0xffff1383u, "" },
};
inline constexpr Seg fxColour_segs[] = {
  { "colPos", { 241,104,158,26 }, 37, 3, false, "Where Drive, Transistor and Fuzz sit. PRE: before the filter, so the filter shapes the grit (smooth, classic synth). POST: after the filter, so the grit stays bright and fizzy and the cutoff only shapes the base tone. BOTH: a light stage before and the full stage after." },
  { "anaTabs", { 1331,104,308,26 }, 40, 3, false, "" },
};
inline constexpr Item fxColour_items[] = {
  { "note", "Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you switch them on.", "", { 1331,568,308,84 }, { 0,0,0,0 } },
  { "canvas", "curveCv", "", { 241,266,1058,380 }, { 0,0,0,0 } },
  { "canvas", "anaCv", "", { 1331,138,308,220 }, { 0,0,0,0 } },
  { "rail", "", "", { 50,71,170,604 }, { 0,0,0,0 } },
  { "railbtn", "Colour", "drive, fuzz, crush", { 50,71,170,54 }, { 0,0,0,0 } },
  { "railbtn", "EQ", "5 band, mid and side", { 50,131,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Space", "reverb", { 50,191,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Dynamics", "opto, OTT, dance", { 50,251,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Stereo", "width, mid and side", { 50,311,170,54 }, { 0,0,0,0 } },
  { "railnote", "Signal order is fixed: Colour, Filter, Amp, EQ, Space, Opto, OTT, Dance master, Stereo, Limiter.", "", { 50,371,170,60 }, { 0,0,0,0 } },
  { "ana", "", "", { 1320,71,330,604 }, { 0,0,0,0 } },
};
inline constexpr Sec fxEq_secs[] = {
  { "", "GRAPHIC EQ", "DRAG A POINT; WHEEL CHANGES Q; PICK A BAND BELOW", { 230,71,1080,368 } },
  { "fxB1", "BAND 1 LOW SHELF", "100 HZ  0.0 DB", { 230,449,1080,226 } },
};
inline constexpr Knob fxEq_knobs[] = {
  { "eqLoF", "FREQ", { 241,516,62,74 }, { 250,530,44,44 }, 'C', "" },
  { "eqLo", "LOW", { 311,516,62,74 }, { 320,530,44,44 }, 'D', "Shelf at 100 Hz. A small cut here clears mud; a small boost adds weight to a bass." },
  { "eqD1", "DYNAMIC", { 381,516,62,74 }, { 390,530,44,44 }, 'Z', "Makes the band move with the level: 0 keeps it static, higher values add or cut that much more when the signal is loud." },
  { "eqT1", "THRESHOLD", { 451,516,62,74 }, { 466,530,44,44 }, 'K', "" },
};
inline constexpr Tog fxEq_togs[] = {
  { "eqO1", "ON", { 415,482,56,26 }, 0xff02ebbau, "" },
};
inline constexpr Seg fxEq_segs[] = {
  { "bandSel", { 286,104,130,26 }, 43, 5, false, "" },
  { "eqM1", { 241,482,168,26 }, 48, 3, false, "" },
  { "anaTabs", { 1331,104,308,26 }, 51, 3, false, "" },
};
inline constexpr Item fxEq_items[] = {
  { "lab", "Band", "", { 241,110,39,14 }, { 0,0,0,0 } },
  { "note", "Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you switch them on.", "", { 1331,568,308,84 }, { 0,0,0,0 } },
  { "canvas", "eqBig", "", { 241,138,1058,290 }, { 0,0,0,0 } },
  { "canvas", "anaCv", "", { 1331,138,308,220 }, { 0,0,0,0 } },
  { "rail", "", "", { 50,71,170,604 }, { 0,0,0,0 } },
  { "railbtn", "Colour", "drive, fuzz, crush", { 50,71,170,54 }, { 0,0,0,0 } },
  { "railbtn", "EQ", "5 band, mid and side", { 50,131,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Space", "reverb", { 50,191,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Dynamics", "opto, OTT, dance", { 50,251,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Stereo", "width, mid and side", { 50,311,170,54 }, { 0,0,0,0 } },
  { "railnote", "Signal order is fixed: Colour, Filter, Amp, EQ, Space, Opto, OTT, Dance master, Stereo, Limiter.", "", { 50,371,170,60 }, { 0,0,0,0 } },
  { "ana", "", "", { 1320,71,330,604 }, { 0,0,0,0 } },
};
inline constexpr Sec fxSpace_secs[] = {
  { "fxSpace", "SPACE", "SHORT ROOM REVERB", { 230,71,535,418 } },
  { "", "TAIL", "PRE-DELAY, LENGTH, LOW CUT", { 775,71,535,418 } },
};
inline constexpr Knob fxSpace_knobs[] = {
  { "verbMix", "MIX", { 241,138,62,74 }, { 250,152,44,44 }, 'P', "Wet level. The return is high-passed so the low end never smears." },
  { "verbSize", "LENGTH", { 311,138,62,74 }, { 320,152,44,44 }, 'V', "Kept short by default. Long tails are for leads." },
  { "verbHP", "LOW CUT", { 381,138,62,74 }, { 390,152,44,44 }, 'H', "" },
  { "spPre", "PRE-DELAY", { 451,138,62,74 }, { 463,152,44,44 }, 'M', "Gap before the tail starts." },
  { "spDamp", "DAMPING", { 521,138,62,74 }, { 530,152,44,44 }, 'P', "Darkens the tail." },
  { "spWid", "WIDTH", { 591,138,62,74 }, { 600,152,44,44 }, 'P', "Stereo spread of the tail." },
  { "spDuck", "DUCKING", { 661,138,62,74 }, { 670,152,44,44 }, 'P', "Lowers the tail while notes play." },
};
inline constexpr Tog fxSpace_togs[] = {
  { "fxOnSpace", "ON", { 241,104,56,26 }, 0xff02ebbau, "" },
};
inline constexpr Seg fxSpace_segs[] = {
  { "anaTabs", { 1331,104,308,26 }, 54, 3, false, "" },
};
inline constexpr Item fxSpace_items[] = {
  { "note", "Ducking lowers the tail while you play and lets it bloom when you stop, so the sound stays tight.", "", { 786,450,513,28 }, { 0,0,0,0 } },
  { "note", "Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you switch them on.", "", { 1331,568,308,84 }, { 0,0,0,0 } },
  { "canvas", "tailCv", "", { 786,104,513,340 }, { 0,0,0,0 } },
  { "canvas", "anaCv", "", { 1331,138,308,220 }, { 0,0,0,0 } },
  { "rail", "", "", { 50,71,170,604 }, { 0,0,0,0 } },
  { "railbtn", "Colour", "drive, fuzz, crush", { 50,71,170,54 }, { 0,0,0,0 } },
  { "railbtn", "EQ", "5 band, mid and side", { 50,131,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Space", "reverb", { 50,191,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Dynamics", "opto, OTT, dance", { 50,251,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Stereo", "width, mid and side", { 50,311,170,54 }, { 0,0,0,0 } },
  { "railnote", "Signal order is fixed: Colour, Filter, Amp, EQ, Space, Opto, OTT, Dance master, Stereo, Limiter.", "", { 50,371,170,60 }, { 0,0,0,0 } },
  { "ana", "", "", { 1320,71,330,604 }, { 0,0,0,0 } },
};
inline constexpr Sec fxDyn_secs[] = {
  { "fxOpto", "OPTO", "SLOW LEVELLER", { 230,71,326,442 } },
  { "fxOtt", "OTT", "UP AND DOWN BY BAND", { 566,71,326,442 } },
  { "fxDm", "DANCE MASTER", "3 BAND COMP, CLIPPER", { 902,71,408,442 } },
  { "fxMax", "MAXIMIZER", "SMOOTH, NO PUMPING", { 230,523,535,152 } },
  { "fxLim", "LIMITER AND LEVEL", "LAST STAGE", { 775,523,535,152 } },
};
inline constexpr Knob fxDyn_knobs[] = {
  { "opto", "OPTO", { 241,138,62,74 }, { 250,152,44,44 }, 'P', "LA-2A style optical leveller. Slow, program dependent, makes bass thick and even without pumping." },
  { "ottAmt", "AMOUNT", { 577,138,62,74 }, { 586,152,44,44 }, 'P', "Mix of the OTT effect." },
  { "ottDepth", "DEPTH", { 647,138,62,74 }, { 656,152,44,44 }, 'P', "" },
  { "ottTime", "TIME", { 717,138,62,74 }, { 726,152,44,44 }, 'P', "" },
  { "ottLo", "LOW", { 787,138,62,74 }, { 796,152,44,44 }, 'D', "" },
  { "ottMid", "MID", { 577,222,62,74 }, { 586,236,44,44 }, 'D', "" },
  { "ottHi", "HIGH", { 647,222,62,74 }, { 656,236,44,44 }, 'D', "" },
  { "dmMix", "MIX", { 913,138,62,74 }, { 922,152,44,44 }, 'P', "Dry and wet blend. 50% is the Dance master starting point." },
  { "dmDrive", "DRIVE", { 983,138,62,74 }, { 992,152,44,44 }, 'P', "" },
  { "dmClip", "CLIP", { 1053,138,62,74 }, { 1062,152,44,44 }, 'P', "Soft clipper before the limiter." },
  { "dmLow", "LOW", { 1123,138,62,74 }, { 1132,152,44,44 }, 'P', "Compression on the low band. Zero keeps the bass untouched (Dance master 2 style)." },
  { "dmMid", "MID", { 1193,138,62,74 }, { 1202,152,44,44 }, 'P', "" },
  { "dmHigh", "HIGH", { 913,222,62,74 }, { 922,236,44,44 }, 'P', "" },
  { "maxAmt", "AMOUNT", { 241,556,62,74 }, { 250,570,44,44 }, 'P', "Gentle upward and downward compression. Smooth, no pumping." },
  { "limCeil", "CEILING", { 786,590,62,74 }, { 795,604,44,44 }, 'D', "Highest level the limiter lets through." },
  { "vol", "VOLUME", { 856,590,62,74 }, { 865,604,44,44 }, 'P', "" },
};
inline constexpr Tog fxDyn_togs[] = {
  { "ottOn", "ON", { 577,104,56,26 }, 0xffff1383u, "" },
  { "dmOn", "ON", { 913,104,56,26 }, 0xffff1383u, "" },
  { "safety", "SAFETY LIMITER", { 786,556,148,26 }, 0xff02ebbau, "Last stage. Keeps the output under -1 dBFS." },
};
inline constexpr Seg fxDyn_segs[] = {
  { "optoSpd", { 241,104,160,26 }, 57, 3, false, "How fast the optical leveller reacts." },
  { "anaTabs", { 1331,104,308,26 }, 60, 3, false, "" },
};
inline constexpr Item fxDyn_items[] = {
  { "note", "Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you switch them on.", "", { 1331,568,308,84 }, { 0,0,0,0 } },
  { "canvas", "grCv", "", { 913,304,386,110 }, { 0,0,0,0 } },
  { "canvas", "anaCv", "", { 1331,138,308,220 }, { 0,0,0,0 } },
  { "rail", "", "", { 50,71,170,604 }, { 0,0,0,0 } },
  { "railbtn", "Colour", "drive, fuzz, crush", { 50,71,170,54 }, { 0,0,0,0 } },
  { "railbtn", "EQ", "5 band, mid and side", { 50,131,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Space", "reverb", { 50,191,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Dynamics", "opto, OTT, dance", { 50,251,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Stereo", "width, mid and side", { 50,311,170,54 }, { 0,0,0,0 } },
  { "railnote", "Signal order is fixed: Colour, Filter, Amp, EQ, Space, Opto, OTT, Dance master, Stereo, Limiter.", "", { 50,371,170,60 }, { 0,0,0,0 } },
  { "ana", "", "", { 1320,71,330,604 }, { 0,0,0,0 } },
};
inline constexpr Sec fxStereo_secs[] = {
  { "fxWid", "WIDTH", "MID AND SIDE", { 230,71,535,604 } },
  { "", "STEREO FIELD", "VECTORSCOPE; A MONO SOUND IS ONE VERTICAL LINE", { 775,71,535,604 } },
};
inline constexpr Knob fxStereo_knobs[] = {
  { "stW", "WIDTH", { 241,138,76,74 }, { 257,152,44,44 }, 'P', "Overall stereo width. 100% leaves the sound as it is." },
  { "stWLo", "WIDTH LOW", { 325,138,76,74 }, { 341,152,44,44 }, 'P', "Width below the split. Keep it small so the bass stays solid." },
  { "stWHi", "WIDTH HIGH", { 409,138,76,74 }, { 425,152,44,44 }, 'P', "Width above the split." },
  { "stMono", "MONO BELOW", { 493,138,76,74 }, { 513,152,44,44 }, 'C', "Everything under this frequency is summed to the centre. 20 Hz means off." },
  { "stMid", "MID LEVEL", { 241,222,76,74 }, { 257,236,44,44 }, 'D', "" },
  { "stSide", "SIDE LEVEL", { 325,222,76,74 }, { 341,236,44,44 }, 'D', "" },
  { "stMidDrv", "MID DRIVE", { 409,222,76,74 }, { 425,236,44,44 }, 'P', "Saturation on the mid signal." },
  { "stSideDrv", "SIDE DRIVE", { 493,222,76,74 }, { 509,236,44,44 }, 'P', "Saturation on the side signal." },
};
inline constexpr Tog fxStereo_togs[] = {
  { "stOn", "ON", { 241,104,56,26 }, 0xff02ebbau, "" },
};
inline constexpr Seg fxStereo_segs[] = {
  { "anaTabs", { 1331,104,308,26 }, 63, 3, false, "" },
};
inline constexpr Item fxStereo_items[] = {
  { "note", "Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you switch them on.", "", { 1331,568,308,84 }, { 0,0,0,0 } },
  { "canvas", "fieldCv", "", { 786,104,513,480 }, { 0,0,0,0 } },
  { "canvas", "anaCv", "", { 1331,138,308,220 }, { 0,0,0,0 } },
  { "rail", "", "", { 50,71,170,604 }, { 0,0,0,0 } },
  { "railbtn", "Colour", "drive, fuzz, crush", { 50,71,170,54 }, { 0,0,0,0 } },
  { "railbtn", "EQ", "5 band, mid and side", { 50,131,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Space", "reverb", { 50,191,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Dynamics", "opto, OTT, dance", { 50,251,170,54 }, { 0,0,0,0 } },
  { "railbtn", "Stereo", "width, mid and side", { 50,311,170,54 }, { 0,0,0,0 } },
  { "railnote", "Signal order is fixed: Colour, Filter, Amp, EQ, Space, Opto, OTT, Dance master, Stereo, Limiter.", "", { 50,371,170,60 }, { 0,0,0,0 } },
  { "ana", "", "", { 1320,71,330,604 }, { 0,0,0,0 } },
};
inline constexpr Sec set_secs[] = {
  { "stBanks", "BANKS", "A TO G, SHARE AS A FILE", { 50,71,483,356 } },
  { "stTune", "TUNING", "TEMPERAMENT AND A4", { 543,71,658,356 } },
  { "stRnd", "RANDOM LFO", "SAMPLE AND HOLD", { 1211,71,439,356 } },
  { "stQual", "QUALITY", "SOUND ENGINE", { 50,437,483,238 } },
  { "stMidi", "MIDI IN", "WHEEL, AFTERTOUCH", { 543,437,658,238 } },
  { "stWin", "WINDOW", "SIZE AND LOOK", { 1211,437,439,238 } },
};
inline constexpr Knob set_knobs[] = {
  { "rndHz", "RATE", { 1222,170,62,74 }, { 1231,184,44,44 }, 'L', "" },
  { "rndSlew", "SLEW", { 1292,170,62,74 }, { 1301,184,44,44 }, 'P', "0 steps, 100 glides between values." },
  { "rndDepth", "DEPTH", { 1362,170,62,74 }, { 1371,184,44,44 }, 'P', "" },
  { "wheelCut", "CUTOFF", { 554,552,62,74 }, { 563,566,44,44 }, 'E', "How far the mod wheel moves the filter cutoff, in octaves at full wheel. Turn left to close the filter." },
  { "wheelVib", "VIBRATO", { 624,552,62,74 }, { 633,566,44,44 }, 'P', "How much vibrato the mod wheel adds to both oscillators. It follows the LFO rate." },
  { "wheelVerb", "REVERB", { 694,552,62,74 }, { 703,566,44,44 }, 'P', "How much reverb the mod wheel adds on top of the Space mix." },
  { "atCut", "CUTOFF", { 880,552,62,74 }, { 889,566,44,44 }, 'E', "How far channel aftertouch (pressure) moves the filter cutoff, in octaves at full pressure. Needs a keyboard that sends pressure." },
  { "atVib", "VIBRATO", { 950,552,62,74 }, { 959,566,44,44 }, 'P', "How much vibrato aftertouch adds to both oscillators." },
  { "atVerb", "REVERB", { 1020,552,62,74 }, { 1029,566,44,44 }, 'P', "How much reverb aftertouch adds on top of the Space mix." },
};
inline constexpr Tog set_togs[] = {
  { "rndCut", "CUTOFF", { 1222,104,90,26 }, 0xff02ebbau, "" },
  { "rndPA", "PITCH A", { 1318,104,90,26 }, 0xffffd100u, "" },
  { "rndPB", "PITCH B", { 1415,104,91,26 }, 0xffffd100u, "" },
  { "rndPW", "PULSE WIDTH", { 1222,136,129,26 }, 0xff6cc9f5u, "" },
};
inline constexpr Seg set_segs[] = {
  { "tuneTemp", { 554,104,317,26 }, 66, 4, false, "Equal is standard tuning. Just and Pythagorean pull the notes onto pure intervals. Custom lets you drag single notes." },
  { "tuneRef", { 877,104,125,26 }, 70, 3, false, "Reference pitch of the note A4." },
  { "oversmp", { 61,470,172,26 }, 73, 2, false, "8x is cleaner on high notes and uses more CPU. You choose." },
  { "bend", { 554,470,120,26 }, 75, 4, false, "Pitch wheel range in semitones, up and down. 12 is one octave, 24 is two." },
  { "wheelCC", { 679,470,99,26 }, 79, 2, false, "Which controller drives the Wheel knob." },
  { "winSize", { 1327,470,249,26 }, 81, 5, false, "" },
};
inline constexpr Item set_items[] = {
  { "lab", "Window size", "", { 1222,476,99,14 }, { 0,0,0,0 } },
  { "lab", "Mod wheel", "", { 554,532,120,14 }, { 0,0,0,0 } },
  { "lab", "Aftertouch", "", { 880,532,120,14 }, { 0,0,0,0 } },
  { "note", "A bank is one file with up to 128 sounds. The factory sounds are bank A and stay read only; your own saves go to bank B.", "", { 61,377,461,28 }, { 0,0,0,0 } },
  { "note", "Cents each note is moved, -50 to +50, from the key C. Drag a bar to pull a note out of tune; that switches to Custom.", "", { 554,296,636,28 }, { 0,0,0,0 } },
  { "note", "Normal runs the engine at 4x oversampling. High runs it at 8x: cleaner highs on loud, fuzzy sounds, about double the CPU for the audio part.", "", { 61,504,461,28 }, { 0,0,0,0 } },
  { "note", "MIDI out for the sequencer and arp lives on the Seq / Arp page, off by default.", "", { 554,504,636,14 }, { 0,0,0,0 } },
  { "note", "You can also drag any edge or corner, or double-click the wood on either side to go back to 100%.", "", { 1222,504,417,28 }, { 0,0,0,0 } },
  { "list", "stBanks", "", { 61,143,461,150 }, { 0,0,0,0 } },
  { "bankbtn", "", "", { 61,104,461,31 }, { 0,0,0,0 } },
  { "cents", "", "", { 554,138,636,152 }, { 0,0,0,0 } },
  { "btn", "Import bank", "", { 61,301,127,31 }, { 0,0,0,0 } },
  { "btn", "Export bank", "", { 194,301,130,31 }, { 0,0,0,0 } },
  { "btn", "Import sound", "", { 330,301,138,31 }, { 0,0,0,0 } },
  { "btn", "Export sound", "", { 61,338,141,31 }, { 0,0,0,0 } },
};
struct ViewData { const char* name; const Sec* secs; int nSecs; const Knob* knobs; int nKnobs; const Tog* togs; int nTogs; const Seg* segs; int nSegs; const Item* items; int nItems; };
inline constexpr ViewData views[] = {
  { "seq", seq_secs, (int) std::size(seq_secs), seq_knobs, (int) std::size(seq_knobs), seq_togs, (int) std::size(seq_togs), seq_segs, (int) std::size(seq_segs), seq_items, (int) std::size(seq_items) },
  { "fxColour", fxColour_secs, (int) std::size(fxColour_secs), fxColour_knobs, (int) std::size(fxColour_knobs), fxColour_togs, (int) std::size(fxColour_togs), fxColour_segs, (int) std::size(fxColour_segs), fxColour_items, (int) std::size(fxColour_items) },
  { "fxEq", fxEq_secs, (int) std::size(fxEq_secs), fxEq_knobs, (int) std::size(fxEq_knobs), fxEq_togs, (int) std::size(fxEq_togs), fxEq_segs, (int) std::size(fxEq_segs), fxEq_items, (int) std::size(fxEq_items) },
  { "fxSpace", fxSpace_secs, (int) std::size(fxSpace_secs), fxSpace_knobs, (int) std::size(fxSpace_knobs), fxSpace_togs, (int) std::size(fxSpace_togs), fxSpace_segs, (int) std::size(fxSpace_segs), fxSpace_items, (int) std::size(fxSpace_items) },
  { "fxDyn", fxDyn_secs, (int) std::size(fxDyn_secs), fxDyn_knobs, (int) std::size(fxDyn_knobs), fxDyn_togs, (int) std::size(fxDyn_togs), fxDyn_segs, (int) std::size(fxDyn_segs), fxDyn_items, (int) std::size(fxDyn_items) },
  { "fxStereo", fxStereo_secs, (int) std::size(fxStereo_secs), fxStereo_knobs, (int) std::size(fxStereo_knobs), fxStereo_togs, (int) std::size(fxStereo_togs), fxStereo_segs, (int) std::size(fxStereo_segs), fxStereo_items, (int) std::size(fxStereo_items) },
  { "set", set_secs, (int) std::size(set_secs), set_knobs, (int) std::size(set_knobs), set_togs, (int) std::size(set_togs), set_segs, (int) std::size(set_segs), set_items, (int) std::size(set_items) },
};
enum View { vSeq, vFxColour, vFxEq, vFxSpace, vFxDyn, vFxStereo, vSet };
} // namespace purrone::layout2
