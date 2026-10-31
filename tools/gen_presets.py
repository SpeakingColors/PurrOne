#!/usr/bin/env python3
"""Generates src/PresetsGen.h (banks A, B and C of 128 factory sounds each, 20 of bank A are the hand made ones in Presets.h)
and src/FxPresets.h (the dropdown presets of each FX module).

Usage: tools/gen_presets.py defaults.txt [levels.tsv]
  defaults.txt  from `PurrOneCheck defaults`
  levels.tsv    optional, `name<TAB>vol` lines from `PurrOneCheck check` (volume that matches the loudness target)

The sounds are built from voices (the synth part), FX chains made of the module presets, and sequences. Everything is
deterministic, so running the script again gives the same files.
"""
import re, sys, os, json

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ------------------------------------------------------------------ defaults
DEF = {}
for line in open(sys.argv[1]):
    k, t, d, lo, hi = line.split()
    DEF[k] = (t, float(d), float(lo), float(hi))
LEVELS = {}
if len(sys.argv) > 2 and os.path.exists(sys.argv[2]):
    for line in open(sys.argv[2]):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2:
            LEVELS[p[0]] = float(p[1])

# ------------------------------------------------------------------ the 20 hand made sounds (parsed, used as voices)
src = open(os.path.join(ROOT, "src", "Presets.h")).read()
CORE = {}
for m in re.finditer(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*\{(.*?)\}\s*\}', src, re.S):
    vals = {}
    for k, v in re.findall(r'\{"(\w+)",\s*(-?[\d.]+)f\}', m.group(3)):
        vals[k] = float(v)
    CORE[m.group(1)] = (m.group(2), vals)

def core(name):
    return dict(CORE[name][1])

# ------------------------------------------------------------------ new voices
def v(**kw):
    return {k: float(x) for k, x in kw.items()}

VOICES = {}   # name -> (category, values)
def add(name, cat, vals): VOICES[name] = (cat, vals)

# sub and the hand made bass / lead / fx sounds
for n in ["Deep Sub", "808 Boom", "Clean Sub Thump", "Rumble Sub"]: add(n, "Sub", core(n))
for n in ["Round Bass", "Dirty Reese", "Nasty 808 Dub", "Pluck Bass", "Acid Squelch", "Dub Wobble", "Reese Growl", "Saw Wub", "Rolling Bass", "Fuzz Bass", "Talk Bass"]: add(n, "Bass", core(n))
for n in ["Sync Lead", "Soft Lead", "Hollow Pulse"]: add(n, "Lead", core(n))
add("Drone Texture", "FX", core("Drone Texture"))

# new bass voices. Every value is explicit where it differs from the default sound (the aggressive Dirty Reese start).
add("Square Acid", "Bass", v(aSaw=0, aPulse=1, aPW=0.5, bSaw=0, mixA=0.9, mixB=0, mixSub=0.1, cutoff=380, reso=0.8, fenvAmt=0.7, fD=0.32, fS=0, aD=0.4, aS=0.7,
    glide=0.2, glideAuto=1, xstr=0.3, fuzz=0.1, kbd=0.2, aOct=1, bOct=1, eqMid=0, eqHi=-1, opto=0.2))
add("Electro Bass", "Bass", v(aSaw=0, aPulse=1, aPW=0.4, bSaw=0, bPulse=1, bPW=0.55, bTune=0.1, mixA=0.7, mixB=0.5, mixSub=0.5, cutoff=520, reso=0.35, fenvAmt=0.6, fD=0.24, fS=0.05,
    aD=0.3, aS=0.5, aR=0.15, xstr=0.4, drive=0.15, fuzz=0.2, kbd=0.4, aOct=1, bOct=1, eqMid=1, eqHi=-3, opto=0.4))
add("Sync Bass", "Bass", v(aSaw=1, aSync=1, bSaw=1, bTune=7, bOct=1, aOct=0, mixA=0.8, mixB=0.2, mixSub=0.5, cutoff=900, reso=0.3, fenvAmt=0.5, fD=0.3, fS=0.2,
    aS=0.9, xstr=0.45, fuzz=0.3, kbd=0.4, eqMid=1.5, eqHi=-3, opto=0.5))
add("PWM Bass", "Bass", v(aSaw=0, aPulse=1, aPW=0.5, bSaw=0, bPulse=1, bPW=0.5, bTune=0.15, aOct=0, bOct=0, mixA=0.7, mixB=0.6, mixSub=0.4, cutoff=700, reso=0.25,
    fenvAmt=0.3, fD=0.4, fS=0.4, aS=1.0, lfoHz=0.7, mLfo=0.7, mLfoR=1, dest1=1, dest3=1, xstr=0.4, fuzz=0.2, kbd=0.4, eqMid=1.5, eqHi=-3, opto=0.5))
add("Noise Bass", "Bass", v(aSaw=1, bSaw=0, mixA=0.8, mixB=0, mixSub=0.5, mixNoise=0.3, aOct=1, bOct=1, cutoff=450, reso=0.4, fenvAmt=0.55, fD=0.28, fS=0.05,
    aD=0.3, aS=0.4, aR=0.15, xstr=0.45, drive=0.2, fuzz=0.4, kbd=0.4, eqMid=1, eqHi=-4, opto=0.5))
add("Neuro Bass", "Bass", v(aSaw=1, bSaw=1, bTune=0.7, aOct=0, bOct=0, mixA=0.7, mixB=0.7, mixSub=0.3, cutoff=480, reso=0.5, fenvAmt=0.3, fD=0.5, fS=0.4, aS=1.0,
    lfoHz=3.0, lfoTri=0, lfoSaw=1, mLfo=0.55, mLfoR=1, dest4=1, dest2=1, mOscB=0.12, mOscBR=1, xstr=0.55, drive=0.25, fuzz=0.55, colPos=1, kbd=0.4, eqLo=1, eqMid=1.5, eqHi=-3, opto=0.6))
add("Garage Bass", "Bass", v(fMode=2, aSaw=1, bSaw=0, bPulse=1, bPW=0.4, bTune=0.1, aOct=1, bOct=1, mixA=0.8, mixB=0.3, mixSub=0.6, cutoff=380, reso=0.5, fenvAmt=0.5, fD=0.3, fS=0.15,
    aD=0.4, aS=0.6, xstr=0.35, fuzz=0.15, kbd=0.6, eqLo=1.5, eqMid=1.5, eqHi=-3, opto=0.5))
add("Techno Rumble", "Bass", v(aSaw=1, bSaw=0, mixA=0.6, mixB=0, mixSub=0.8, aOct=0, bOct=0, cutoff=260, reso=0.3, fenvAmt=0.35, fD=0.2, fS=0.05, aD=0.28, aS=0.35, aR=0.2,
    xstr=0.5, drive=0.3, fuzz=0.25, colPos=1, kbd=0.2, eqLo=2, eqMid=0, eqHi=-5, opto=0.6))
add("Dub Stab", "Bass", v(aSaw=1, bSaw=1, bTune=0.3, aOct=1, bOct=1, mixA=0.7, mixB=0.6, mixSub=0.2, cutoff=900, reso=0.35, fenvAmt=0.8, fD=0.25, fS=0, aD=0.3, aS=0.15, aR=0.2,
    xstr=0.3, fuzz=0.15, kbd=0.5, eqMid=1, eqHi=-2, opto=0.4, verbMix=0.2, verbSize=1.6))
add("Octave Bass", "Bass", v(aSaw=1, bSaw=1, bTune=0, aOct=0, bOct=1, mixA=0.75, mixB=0.55, mixSub=0.3, cutoff=620, reso=0.25, fenvAmt=0.4, fD=0.35, fS=0.3, aS=0.9,
    xstr=0.4, fuzz=0.25, kbd=0.5, eqMid=1.5, eqHi=-3, opto=0.5))
add("Slab Saw", "Bass", v(aSaw=1, bSaw=1, bTune=0.85, aOct=0, bOct=0, mixA=0.7, mixB=0.7, mixSub=0.5, cutoff=1100, reso=0.2, fenvAmt=0.3, fD=0.5, fS=0.5, aS=1.0,
    xstr=0.5, drive=0.2, fuzz=0.45, colPos=2, kbd=0.5, eqLo=1, eqMid=1, eqHi=-3, opto=0.6))
add("Wobble Pulse", "Bass", v(aSaw=0, aPulse=1, aPW=0.35, bSaw=0, bPulse=1, bPW=0.6, bTune=0.2, aOct=0, bOct=0, mixA=0.7, mixB=0.5, mixSub=0.5, cutoff=300, reso=0.6, fenvAmt=0.1,
    fS=0.5, aS=1.0, lfoHz=2.5, lfoTri=1, mLfo=0.75, mLfoR=1, dest4=1, xstr=0.4, drive=0.1, fuzz=0.2, colPos=0, kbd=0.3, eqLo=2, eqMid=0, eqHi=-3, opto=0.6))

# new leads
add("Scream Lead", "Lead", v(aSaw=1, aSync=1, bSaw=1, aTune=7, bOct=1, aOct=1, mixA=0.8, mixB=0.3, mixSub=0, cutoff=5200, reso=0.4, fenvAmt=0.3, fD=0.5, fS=0.4,
    aS=0.9, glide=0.12, xstr=0.45, drive=0.2, fuzz=0.6, colPos=0, kbd=0.5, eqMid=1, eqHi=-2, opto=0.4, verbMix=0.18, verbSize=1.4))
add("Acid Lead", "Lead", v(aSaw=1, bSaw=0, mixA=0.9, mixB=0, mixSub=0, aOct=2, bOct=2, cutoff=1800, reso=0.72, fenvAmt=0.6, fD=0.3, fS=0.05, aD=0.4, aS=0.6,
    glide=0.18, glideAuto=1, xstr=0.3, fuzz=0.1, kbd=0.4, eqMid=0, eqHi=-1, opto=0.2, verbMix=0.16, verbSize=1.2))
add("Rave Stab", "Lead", v(aSaw=1, aSync=1, bSaw=1, aTune=5, bTune=0.2, aOct=2, bOct=2, mixA=0.7, mixB=0.5, mixSub=0, cutoff=3200, reso=0.3, fenvAmt=0.6, fD=0.25, fS=0,
    aD=0.28, aS=0.15, aR=0.2, xstr=0.4, fuzz=0.3, kbd=0.6, eqMid=1, eqHi=-1, opto=0.4, verbMix=0.2, verbSize=1.0))
add("Pluck Lead", "Lead", v(aSaw=0, aPulse=1, aPW=0.35, bSaw=0, bPulse=1, bPW=0.6, bTune=0.1, aOct=2, bOct=2, mixA=0.7, mixB=0.5, mixSub=0, cutoff=1800, reso=0.2, fenvAmt=0.7,
    fD=0.25, fS=0, aD=0.3, aS=0.1, aR=0.25, xstr=0.2, fuzz=0, kbd=0.7, eqMid=0, eqHi=-1, opto=0.2, verbMix=0.22, verbSize=1.4))
add("Siren", "Lead", v(aSaw=0, aPulse=1, aPW=0.5, bSaw=0, bTri=1, aOct=2, bOct=2, bTune=0, mixA=0.7, mixB=0.4, mixSub=0, cutoff=4200, reso=0.3, fenvAmt=0.1, fS=0.6,
    aS=1.0, lfoHz=0.9, lfoTri=1, mLfo=0.35, mLfoR=1, dest0=1, dest2=1, xstr=0.2, fuzz=0.1, kbd=0.5, eqMid=0, eqHi=-1, opto=0.2, verbMix=0.25, verbSize=2.2))

# new fx sounds
add("Riser", "FX", v(aSaw=1, bSaw=1, bTune=0.6, mixA=0.6, mixB=0.6, mixNoise=0.35, mixSub=0, aOct=1, bOct=1, cutoff=120, reso=0.55, fenvAmt=0.95, fA=0.9, fD=0.9, fS=1.0,
    aA=0.55, aS=1.0, aR=0.5, xstr=0.3, fuzz=0.2, kbd=0.2, eqMid=0, eqHi=-1, opto=0.3, verbMix=0.3, verbSize=2.5))
add("Laser Zap", "FX", v(aSaw=1, bSaw=0, mixA=0.9, mixB=0, mixSub=0, aOct=2, bOct=2, cutoff=2400, reso=0.6, fenvAmt=0.2, fD=0.4, fS=0, aD=0.5, aS=0, aR=0.2,
    mEnv=0.9, mEnvR=1, dest0=1, xstr=0.3, fuzz=0.15, kbd=0.2, eqMid=0, eqHi=0, opto=0.2, verbMix=0.2, verbSize=1.2))
add("Noise Hit", "FX", v(aSaw=0, bSaw=0, mixA=0, mixB=0, mixSub=0, mixNoise=1.0, fMode=2, cutoff=1800, reso=0.45, fenvAmt=0.5, fD=0.4, fS=0, aD=0.5, aS=0, aR=0.2,
    xstr=0.3, fuzz=0.1, kbd=0.3, eqMid=0, eqHi=0, opto=0.3, verbMix=0.25, verbSize=1.0))

assert len(VOICES) == 39, len(VOICES)

# ------------------------------------------------------------------ module presets (also the dropdown of each module)
FX = {}
def mod(module, name, **kw): FX.setdefault(module, []).append((name, {k: float(x) for k, x in kw.items()}))

mod("Colour", "Clean", drive=0, xstr=0.1, fuzz=0, colPos=2)
mod("Colour", "Warm", drive=0.1, xstr=0.35, fuzz=0, colPos=2)
mod("Colour", "Gritty", drive=0.2, xstr=0.5, fuzz=0.35, colPos=2)
mod("Colour", "Nasty", drive=0.3, xstr=0.6, fuzz=0.6, colPos=1)
mod("Colour", "Fuzz Wall", drive=0.35, xstr=0.6, fuzz=0.8, colPos=0)
mod("Colour", "Tube Push", drive=0.45, xstr=0.3, fuzz=0, colPos=1)

_off = dict(crushOn=0)
mod("Crush", "Off", crushOn=0)
mod("Crush", "Subtle 12 bit", crushOn=1, crushBits=12, crushRate=36000, crushMix=0.35)
mod("Crush", "Lo-fi 8 bit", crushOn=1, crushBits=8, crushRate=24000, crushMix=0.5)
mod("Crush", "Radio", crushOn=1, crushBits=10, crushRate=11000, crushMix=0.6)
mod("Crush", "Crunch 6 bit", crushOn=1, crushBits=6, crushRate=16000, crushMix=0.6)
mod("Crush", "Alias 4 bit", crushOn=1, crushBits=4, crushRate=8000, crushMix=0.7)

def eqflat(**kw):
    base = dict(eqLo=0, eqMid=0, eqHi=0, eqG2=0, eqG4=0, eqD1=0, eqD2=0, eqD3=0, eqD4=0, eqD5=0, eqM1=0, eqM2=0, eqM3=0, eqM4=0, eqM5=0)
    base.update(kw); return base
mod("EQ", "Flat", **eqflat())
mod("EQ", "Classic", **eqflat(eqMid=2.3, eqHi=-3))
mod("EQ", "Tight Low End", **eqflat(eqG2=-3, eqF2=250, eqQ2=1.2, eqHi=-2, eqMid=1))
mod("EQ", "Mud Cut", **eqflat(eqG2=-4, eqF2=300, eqQ2=1.0, eqMid=1.5, eqHi=-2))
mod("EQ", "Presence", **eqflat(eqG4=3, eqF4=3200, eqQ4=1.0, eqMid=1, eqHi=-1))
mod("EQ", "Dark", **eqflat(eqLo=1.5, eqHi=-8, eqHiF=4000))
mod("EQ", "Bright", **eqflat(eqMid=1, eqHi=4, eqHiF=8000))
mod("EQ", "Dynamic Tame", **eqflat(eqMid=1.5, eqG4=-2, eqF4=3200, eqQ4=1.0, eqD4=-6, eqT4=-30, eqHi=-2))
mod("EQ", "Side Air", **eqflat(eqMid=1, eqHi=4, eqHiF=8000, eqM5=2))
mod("EQ", "Mono Low End", **eqflat(eqLo=-12, eqLoF=160, eqM1=2, eqMid=1.5, eqHi=-2))
mod("EQ", "Scoop", **eqflat(eqLo=3, eqMid=-5, eqMidF=800, eqMidQ=1.0, eqHi=3))

mod("Space", "Dry", verbMix=0)
mod("Space", "Room", verbMix=0.12, verbSize=0.6, spPre=6, spDamp=0.5, spWid=0.8, spDuck=0, fxOnSpace=1)
mod("Space", "Plate", verbMix=0.2, verbSize=1.4, spPre=12, spDamp=0.35, spWid=1.0, spDuck=0, fxOnSpace=1)
mod("Space", "Hall", verbMix=0.25, verbSize=2.8, spPre=25, spDamp=0.5, spWid=1.0, spDuck=0.2, fxOnSpace=1)
mod("Space", "Dub Tail", verbMix=0.3, verbSize=3.5, spPre=40, spDamp=0.7, spWid=0.9, spDuck=0.6, fxOnSpace=1)
mod("Space", "Dark Cave", verbMix=0.3, verbSize=3.8, spPre=20, spDamp=0.9, spWid=1.0, spDuck=0.3, fxOnSpace=1)
mod("Space", "Bright Splash", verbMix=0.22, verbSize=1.0, spPre=0, spDamp=0.1, spWid=1.0, spDuck=0, fxOnSpace=1)

mod("Opto", "Off", opto=0)
mod("Opto", "Smooth", opto=0.3, optoSpd=1)
mod("Opto", "Leveller", opto=0.5, optoSpd=0)
mod("Opto", "Squash", opto=0.8, optoSpd=1)
mod("Opto", "Fast Grab", opto=0.6, optoSpd=2)

mod("OTT", "Off", ottOn=0)
mod("OTT", "Light", ottOn=1, ottAmt=0.25, ottDepth=0.4, ottTime=0.5, ottLo=0, ottMid=0, ottHi=0)
mod("OTT", "Classic", ottOn=1, ottAmt=0.5, ottDepth=0.6, ottTime=0.4, ottLo=0, ottMid=0, ottHi=0)
mod("OTT", "Full", ottOn=1, ottAmt=1.0, ottDepth=0.8, ottTime=0.35, ottLo=0, ottMid=0, ottHi=0)
mod("OTT", "Low Heavy", ottOn=1, ottAmt=0.6, ottDepth=0.6, ottTime=0.4, ottLo=3, ottMid=0, ottHi=-2)
mod("OTT", "Air Lift", ottOn=1, ottAmt=0.5, ottDepth=0.6, ottTime=0.4, ottLo=0, ottMid=0, ottHi=4)

mod("Dance master", "Off", dmOn=0)
mod("Dance master", "Standard", dmOn=1, dmMix=0.5, dmDrive=0.3, dmClip=0.3, dmLow=0, dmMid=0.5, dmHigh=0.4)
mod("Dance master", "Bass Safe", dmOn=1, dmMix=0.6, dmDrive=0.2, dmClip=0.4, dmLow=0, dmMid=0.6, dmHigh=0.5)
mod("Dance master", "Loud", dmOn=1, dmMix=0.8, dmDrive=0.5, dmClip=0.6, dmLow=0.3, dmMid=0.7, dmHigh=0.6)
mod("Dance master", "Smash", dmOn=1, dmMix=0.8, dmDrive=0.5, dmClip=0.6, dmLow=0.5, dmMid=0.8, dmHigh=0.7)

mod("Maximizer", "Off", maxAmt=0)
mod("Maximizer", "Glue", maxAmt=0.25)
mod("Maximizer", "Big", maxAmt=0.5)
mod("Maximizer", "Max", maxAmt=0.8)

mod("Stereo", "Off", stOn=0)
mod("Stereo", "Wide", stOn=1, stW=1.3, stWLo=0.5, stWHi=1.2, stMono=120, stMid=0, stSide=0, stMidDrv=0, stSideDrv=0)
mod("Stereo", "Extra Wide", stOn=1, stW=1.7, stWLo=0.4, stWHi=1.5, stMono=150, stMid=0, stSide=0, stMidDrv=0, stSideDrv=0)
mod("Stereo", "Narrow", stOn=1, stW=0.6, stWLo=1, stWHi=1, stMono=20, stMid=0, stSide=0, stMidDrv=0, stSideDrv=0)
mod("Stereo", "Mono Bass", stOn=1, stW=1.0, stWLo=0, stWHi=1, stMono=150, stMid=0, stSide=0, stMidDrv=0, stSideDrv=0)
mod("Stereo", "Side Grit", stOn=1, stW=1.2, stWLo=0.5, stWHi=1.2, stMono=120, stMid=0, stSide=1.5, stMidDrv=0, stSideDrv=0.4)
mod("Stereo", "Mid Punch", stOn=1, stW=1.0, stWLo=1, stWHi=1, stMono=20, stMid=2, stSide=0, stMidDrv=0.3, stSideDrv=0)

def fxp(module, name):
    for n, vals in FX[module]:
        if n == name: return dict(vals)
    raise KeyError((module, name))

def chain(**kw):
    out = {}
    for module, name in kw.items():
        out.update(fxp(module.replace("_", " "), name))
    return out

# ------------------------------------------------------------------ sequences
NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
def spec(tokens, root):
    out = []
    for t in tokens:
        t = str(t)
        if t == "-": out.append("-"); continue
        m = re.match(r"(-?\d+)([as]*)$", t); semi = int(m.group(1)) + root
        name = NOTES[semi % 12] + str(2 + semi // 12)
        out.append(name + m.group(2))
    return " ".join(out)

PATS = {
 "Run":      ("0 0 12 0 3a 0 10s 12 0 0 7s 7a 0 15 0 -", 2, 0.55, 0.5),
 "Chug":     ("0 - 0 12 - 0 3a 0 7s 0 - 12 10a 0 12s 7", 2, 0.5, 0.5),
 "Drive":    ("0 0 0 12a 0 0 3s 0 0 0 5a 0 0 7s 0 0", 2, 0.6, 0.5),
 "Melody":   ("0 - 3 5a 7s 5 3 - 0 - 10s 8a 7 - 5s 3", 2, 0.55, 0.5),
 "Long Acid":("0 0 12 0 3a 0 10s 12 0 0 7s 7a 0 15 0 - 0 - 0 12 - 0 3a 0 7s 0 - 12 10a 0 12s 7", 2, 0.55, 0.5),
 "Offbeat":  ("- - 0 - - - 0 - - - 0 - - - 0 -", 2, 0.5, 0.5),
 "Offbeat 2":("- - 0 - - - 12 - - - 0 - - - 7 -", 2, 0.5, 0.5),
 "Rolling":  ("0 0 12 0 0 0 12 0 0 0 12 0 0 12 0 7", 2, 0.8, 0.5),
 "Gallop":   ("0 - 0 0 0 - 0 0 0 - 0 0 0 - 0 0", 2, 0.6, 0.5),
 "Pulse":    ("0", 2, 0.5, 0.5),
 "Rolling 32":("0 0 0 0 0 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7 0 0 0 0 0 10 12", 4, 0.8, 0.5),
 "Techno":   ("0 - 0 - 0 0 - 0 0 - 0 - 0 0 - 0", 2, 0.5, 0.5),
 "Electro":  ("0 - - 0 - 0 - - 7 - - 0 3 - 5 -", 2, 0.4, 0.56),
 "Skank":    ("- - 0 - - - - 0 - - 0 - - - - -", 2, 0.35, 0.5),
 "Bounce":   ("0 12 0 12 0 12 0 12 3 15 3 15 5 17 5 17", 2, 0.5, 0.5),
 "Psy":      ("- 0 0 0 - 0 0 0 - 0 0 0 - 0 0 0", 2, 0.7, 0.5),
 "Riff":     ("12 - 15 17 - 19 - 15 24 - 22 19 - 17 15 -", 2, 0.5, 0.5),
 "Trance":   ("12 12 15 12 19 12 15 12 17 17 20 17 24 17 20 17", 2, 0.5, 0.5),
 "Stab":     ("12 - - 12a - - 12 - 15 - - 12 - 17a - -", 2, 0.3, 0.5),
 "Climb":    ("0 3 7 12 15 19 24 27 24 19 15 12 7 3 0 -", 2, 0.5, 0.5),
}
ROOTS = [0, 3, 5, 7, 10, 2, 8, 1]   # C D# F G A# D G# C#

# ------------------------------------------------------------------ banks
def pick_root(i): return ROOTS[i % len(ROOTS)]

def tweak(vals, **kw):
    out = dict(vals)
    for k, x in kw.items(): out[k] = float(x)
    return out

def scaled(vals, key, f, lo=0.0, hi=1.0):
    d = DEF[key][1]
    cur = vals.get(key, d)
    return min(hi, max(lo, cur * f))

def variant_tight(vals):
    o = dict(vals); o.update(chain(EQ="Tight Low End", Opto="Fast Grab"))
    o["fD"] = scaled(vals, "fD", 0.85); o["aR"] = scaled(vals, "aR", 0.7); return o
def variant_heavy(vals):
    o = dict(vals)
    o["drive"] = min(0.9, vals.get("drive", 0) + 0.15); o["xstr"] = min(0.95, vals.get("xstr", DEF["xstr"][1]) + 0.12); o["fuzz"] = min(0.9, vals.get("fuzz", DEF["fuzz"][1]) + 0.18)
    o.update(chain(Maximizer="Big", Opto="Squash")); o["eqLo"] = vals.get("eqLo", 0) + 1.0; return o
def variant_dub(vals):
    o = dict(vals); o.update(chain(Space="Dub Tail", EQ="Mud Cut")); o["opto"] = 0.4
    o["fD"] = scaled(vals, "fD", 1.15); return o

PRESETS = []   # (name, category, values, seq)

def addp(name, cat, vals, seq=""):
    name = name[:30]
    PRESETS.append([name, cat, vals, seq])

voices_by_cat = {c: [n for n, (cc, _) in VOICES.items() if cc == c] for c in ["Sub", "Bass", "Lead", "FX"]}
newvoices = [n for n in VOICES if n not in CORE]
bank = {"A": [], "B": [], "C": [], "D": []}

def mark(b):
    start = len(PRESETS)
    return lambda: bank[b].extend(range(start, len(PRESETS)))

# ---- bank A: the new voices as they are, then Tight / Heavy / Dub versions
m = mark("A")
for n in newvoices: addp(n, VOICES[n][0], VOICES[n][1])
for n in VOICES:
    cat, vals = VOICES[n]; addp(n + " Tight", cat, variant_tight(vals))
heavy_skip = {"Soft Lead", "Siren", "Drone Texture", "Riser", "Laser Zap", "Noise Hit"}
for n in VOICES:
    cat, vals = VOICES[n]
    if n in heavy_skip: continue
    addp(n + " Heavy", cat, variant_heavy(vals))
dub_for = ["Deep Sub", "808 Boom", "Rumble Sub", "Round Bass", "Dirty Reese", "Nasty 808 Dub", "Pluck Bass", "Reese Growl", "Rolling Bass", "Fuzz Bass", "Talk Bass",
           "Dub Stab", "Octave Bass", "Slab Saw", "Techno Rumble", "Neuro Bass"]
for n in dub_for:
    cat, vals = VOICES[n]; v = variant_dub(vals)
    if n == "Neuro Bass": v = dict(v); v["mLfo"] = 0.0   # user request: no LFO on Neuro Bass Dub
    addp(n + " Dub", cat, v)
m()

def seqvals(vals, label, root, extra=None, squelch=False, bpm=138, mode=1):
    tokens, rate, gate, swing = PATS[label]
    toks = tokens.split()
    o = dict(vals)
    o.update(seqMode=mode, seqRate=rate, seqGate=gate, seqSwing=swing, seqBpm=bpm, seqSteps=len(toks))
    if squelch: o.update(squelch=1, squelchAmt=0.7, seqAcc=0.7, seqSlide=0.08)
    if extra: o.update(extra)
    return o, spec(toks, root)

# ---- bank B: sequences (bass 4 each, leads 3, subs 2, fx 2)
STY = {
 "Square Acid": ["Run", "Chug", "Drive", "Melody"], "Acid Squelch": ["Run", "Chug", "Drive", "Long Acid"], "Neuro Bass": ["Rolling", "Gallop", "Psy", "Bounce"],
 "Round Bass": ["Offbeat", "Techno", "Bounce", "Electro"], "Dirty Reese": ["Rolling", "Offbeat 2", "Psy", "Gallop"], "Nasty 808 Dub": ["Pulse", "Skank", "Offbeat", "Bounce"],
 "Pluck Bass": ["Techno", "Electro", "Bounce", "Skank"], "Dub Wobble": ["Skank", "Pulse", "Offbeat", "Bounce"], "Reese Growl": ["Rolling", "Gallop", "Offbeat 2", "Rolling 32"],
 "Saw Wub": ["Pulse", "Gallop", "Offbeat", "Psy"], "Rolling Bass": ["Rolling", "Rolling 32", "Psy", "Gallop"], "Fuzz Bass": ["Gallop", "Techno", "Rolling", "Bounce"],
 "Talk Bass": ["Electro", "Chug", "Techno", "Skank"], "Electro Bass": ["Electro", "Techno", "Bounce", "Run"], "Sync Bass": ["Run", "Rolling", "Techno", "Gallop"],
 "PWM Bass": ["Offbeat 2", "Rolling", "Psy", "Bounce"], "Noise Bass": ["Techno", "Gallop", "Rolling", "Electro"], "Garage Bass": ["Skank", "Bounce", "Offbeat"],
 "Techno Rumble": ["Offbeat", "Techno", "Psy", "Pulse"], "Dub Stab": ["Skank", "Stab", "Offbeat 2", "Bounce"], "Octave Bass": ["Bounce", "Rolling", "Psy"],
 "Slab Saw": ["Gallop", "Rolling", "Techno"], "Wobble Pulse": ["Pulse", "Gallop", "Offbeat"],
}
STY_LEAD = {"Sync Lead": ["Riff", "Trance", "Climb"], "Soft Lead": ["Riff", "Climb", "Melody"], "Hollow Pulse": ["Riff", "Stab", "Trance"], "Scream Lead": ["Riff", "Trance", "Stab"],
            "Acid Lead": ["Melody", "Run", "Chug"], "Rave Stab": ["Stab", "Trance", "Riff"], "Pluck Lead": ["Climb", "Riff", "Trance"], "Siren": ["Climb", "Riff", "Melody"]}
STY_SUB = {"Deep Sub": ["Pulse", "Offbeat"], "808 Boom": ["Pulse", "Skank"], "Clean Sub Thump": ["Techno", "Offbeat"], "Rumble Sub": ["Psy", "Pulse"]}
STY_FX = {"Drone Texture": ["Climb", "Pulse"], "Riser": ["Pulse", "Climb"], "Laser Zap": ["Techno", "Electro"], "Noise Hit": ["Techno", "Skank"]}
SEQ_BPM = {"Run": 132, "Chug": 132, "Drive": 130, "Melody": 128, "Long Acid": 132, "Rolling": 174, "Rolling 32": 174, "Psy": 145, "Gallop": 150, "Skank": 140, "Offbeat": 128, "Offbeat 2": 128,
           "Techno": 135, "Electro": 124, "Bounce": 138, "Pulse": 140, "Riff": 138, "Trance": 140, "Stab": 130, "Climb": 128}
LABEL = {"Melody": "Tune", "Long Acid": "Long Run", "Rolling 32": "Rolling 32nds", "Offbeat 2": "Offbeat Jump"}
ACID_STYLES = {"Run", "Chug", "Drive", "Melody", "Long Acid"}
ctr = 0
KEEP_AS_IS = {"Dirty Reese Gallop", "Nasty 808 Dub Pulse"}      # liked as it is, never touch
def seq_voicing(vals, k):
    """Bank B: the 2nd, 3rd and 4th sequence of a bass get their own voicing so the bank does not sound like one patch four times."""
    o = dict(vals); co = o.get("cutoff", DEF["cutoff"][1]); rs = o.get("reso", DEF["reso"][1]); fe = o.get("fenvAmt", DEF["fenvAmt"][1])
    if k == 1:      # open and bright
        o.update(cutoff=min(6000.0, co * 1.6), reso=min(0.8, rs + 0.12), fenvAmt=min(0.9, fe + 0.15))
    elif k == 2:    # dark, tight and driven
        o.update(cutoff=max(150.0, co * 0.65), fD=scaled(vals, "fD", 0.7), drive=min(0.8, o.get("drive", 0.0) + 0.2), fuzz=min(0.9, o.get("fuzz", DEF["fuzz"][1]) + 0.15))
    elif k == 3:    # other filter mode, B an octave up
        o.update(fMode=2 if o.get("fMode", 0) == 0 else 0, bSemi=12, mixB=min(1.0, o.get("mixB", DEF["mixB"][1]) + 0.1))
    return o

def seq_variants(table, extra_by_style=None):
    global ctr
    for n, styles in table.items():
        cat, vals = VOICES[n]
        for k, st in enumerate(styles):
            squelch = st in ACID_STYLES
            nm = n + " " + LABEL.get(st, st)
            use = vals if (cat != "Bass" or nm in KEEP_AS_IS) else seq_voicing(vals, k)
            o, sp = seqvals(use, st, pick_root(ctr), squelch=squelch, bpm=SEQ_BPM[st])
            if cat == "Lead": o["aOct"] = vals.get("aOct", 0)
            if squelch: o.setdefault("glide", 0)
            ctr += 1
            addp(n + " " + LABEL.get(st, st), "Seq", o, sp)
m = mark("B")
# bass first, then leads, subs and fx; arps for some leads
seq_variants(STY); seq_variants(STY_SUB); seq_variants(STY_FX)
# leads: 2 sequences + 1 arp each
for n, styles in STY_LEAD.items():
    cat, vals = VOICES[n]
    for st in styles[:2]:
        squelch = st in ACID_STYLES
        o, sp = seqvals(vals, st, pick_root(ctr) % 10, squelch=squelch, bpm=SEQ_BPM[st]); ctr += 1
        addp(n + " " + LABEL.get(st, st), "Seq", o, sp)
    o = dict(vals); o.update(seqMode=2, seqRate=2 if ctr % 2 else 0, seqGate=0.5, seqBpm=128, arpPat=ctr % 5, arpOct=1 + ctr % 2, arpLatch=1 if ctr % 3 == 0 else 0)
    ctr += 1; addp(n + " Arp", "Arp", o, "")
m()

# ---- bank C: fx chains
CHAINS = {
 "Wide":      dict(Colour="Gritty", EQ="Mud Cut", Space="Plate", Stereo="Wide", Maximizer="Glue", Opto="Smooth"),
 "OTT":       dict(EQ="Tight Low End", Space="Room", OTT="Classic", Opto="Smooth"),
 "Dance":     dict(EQ="Presence", Space="Room", Dance_master="Bass Safe", Maximizer="Glue"),
 "Crushed":   dict(Crush="Lo-fi 8 bit", EQ="Presence", Space="Room", Opto="Fast Grab"),
 "Neuro OTT": dict(Colour="Nasty", EQ="Dynamic Tame", Space="Dry", OTT="Full", Opto="Smooth"),
 "Industrial":dict(Colour="Fuzz Wall", Crush="Crunch 6 bit", EQ="Scoop", Space="Dark Cave", Dance_master="Loud", Opto="Squash"),
 "Rave":      dict(EQ="Bright", Space="Hall", OTT="Light", Dance_master="Loud", Stereo="Wide"),
 "Stadium":   dict(EQ="Side Air", Space="Hall", OTT="Air Lift", Stereo="Extra Wide", Maximizer="Big"),
 "Radio":     dict(Crush="Radio", EQ="Scoop", Space="Room", Opto="Fast Grab"),
 "Smash":     dict(Colour="Nasty", EQ="Tight Low End", Space="Dry", Dance_master="Smash", Maximizer="Big"),
 "Grit Side": dict(Colour="Gritty", EQ="Mono Low End", Space="Plate", Stereo="Side Grit", OTT="Light"),
 "Alias":     dict(Crush="Alias 4 bit", Colour="Nasty", EQ="Presence", Space="Bright Splash", Opto="Squash"),
}
C_BASS = {  # 4 chains each
 "Round Bass": ["Wide", "OTT", "Dance", "Radio"], "Dirty Reese": ["Neuro OTT", "Smash", "Wide", "Industrial"], "Nasty 808 Dub": ["Smash", "Industrial", "Dance", "Crushed"],
 "Pluck Bass": ["Wide", "Dance", "Crushed", "Grit Side"], "Acid Squelch": ["Crushed", "OTT", "Rave", "Alias"], "Dub Wobble": ["Neuro OTT", "Wide", "Industrial", "Stadium"],
 "Reese Growl": ["Neuro OTT", "Industrial", "Grit Side", "Smash"], "Saw Wub": ["Neuro OTT", "Dance", "Smash", "Wide"], "Rolling Bass": ["Dance", "Rave", "OTT", "Smash"],
 "Fuzz Bass": ["Industrial", "Smash", "Crushed", "Alias"], "Talk Bass": ["OTT", "Crushed", "Radio", "Grit Side"], "Square Acid": ["Crushed", "Rave", "OTT", "Alias"],
 "Electro Bass": ["Radio", "Dance", "Wide", "Crushed"], "Sync Bass": ["OTT", "Rave", "Industrial", "Grit Side"], "PWM Bass": ["Wide", "Stadium", "OTT", "Crushed"],
 "Noise Bass": ["Industrial", "Smash", "Alias", "Crushed"], "Neuro Bass": ["Neuro OTT", "Industrial", "Smash", "Grit Side"], "Garage Bass": ["Wide", "Radio", "Dance"],
 "Techno Rumble": ["OTT", "Industrial", "Dance", "Radio"], "Dub Stab": ["Wide", "Stadium", "Crushed", "OTT"], "Octave Bass": ["Dance", "Wide", "OTT"],
 "Slab Saw": ["Neuro OTT", "Smash", "Stadium"], "Wobble Pulse": ["Neuro OTT", "Wide", "Industrial"],
}
C_LEAD = {"Sync Lead": ["Rave", "Stadium", "Wide"], "Soft Lead": ["Stadium", "Wide", "OTT"], "Hollow Pulse": ["Wide", "Crushed", "Stadium"], "Scream Lead": ["Rave", "Industrial", "Smash"],
          "Acid Lead": ["Crushed", "Rave", "Alias"], "Rave Stab": ["Rave", "Stadium", "OTT"], "Pluck Lead": ["Wide", "Stadium", "Radio"], "Siren": ["Stadium", "Wide", "Alias"]}
C_SUB = {"Deep Sub": ["Dance", "Smash"], "808 Boom": ["Industrial", "Dance"], "Clean Sub Thump": ["OTT", "Smash"], "Rumble Sub": ["Industrial", "OTT"]}
C_FX = {"Drone Texture": ["Stadium", "Industrial"], "Riser": ["Rave", "Stadium"], "Laser Zap": ["Crushed", "Alias"], "Noise Hit": ["Industrial", "Smash"]}
C_SEQ = {"Dirty Reese": "Rolling", "Nasty 808 Dub": "Pulse", "Acid Squelch": "Run", "Reese Growl": "Gallop", "Neuro Bass": "Psy", "Fuzz Bass": "Gallop", "Square Acid": "Chug",
         "Techno Rumble": "Techno", "Rolling Bass": "Rolling 32", "Noise Bass": "Techno", "Electro Bass": "Electro", "Slab Saw": "Rolling"}
def cvariants(table):
    global ctr
    for n, chs in table.items():
        cat, vals = VOICES[n]
        for k, c in enumerate(chs):
            o = dict(vals); o.update(chain(**CHAINS[c]))
            sp = ""; catx = cat
            if n in C_SEQ and k == 0:
                st = C_SEQ[n]; o2, sp = seqvals(o, st, pick_root(ctr), squelch=st in ACID_STYLES, bpm=SEQ_BPM[st]); o = o2; catx = "Seq"; ctr += 1
            addp(n + " " + c, catx, o, sp)
m = mark("C")
cvariants(C_BASS); cvariants(C_LEAD); cvariants(C_SUB); cvariants(C_FX)
m()


# ---- bank D: drum and bass basses (techstep, neuro, reese, liquid, Pendulum, screech, foghorn, laser, jump up, old Pro-One)
# DnB tempo is 174, so LFO rates are given as note values at 174 BPM: 1/4T = 4.35 Hz, 1/8 = 5.8 Hz, 1/8T = 8.7 Hz, 1/16 = 11.6 Hz.
DNB = []   # (name, values)
def dnb(name, fx, **kw):
    base = dict(aSaw=1, aPulse=0, bSaw=1, bPulse=0, bTri=0, aOct=0, bOct=0, aTune=0, bTune=0, aSemi=0, bSemi=0, aSync=0, bLow=0, mixA=0.7, mixB=0.7, mixSub=0.3, mixNoise=0,
                fMode=0, kbd=0.5, fA=0, fD=0.4, fS=0.3, fR=0.2, aA=0, aD=0.4, aS=1.0, aR=0.15, lfoSaw=0, lfoTri=1, lfoPulse=0, mEnv=0, mEnvR=0, mOscB=0, mOscBR=0, mLfo=0, mLfoR=0,
                dest0=0, dest1=0, dest2=0, dest3=0, dest4=0, glide=0, uniVoices=0, uniDetune=0.3, drive=0, xstr=0.4, fuzz=0.3, colPos=2, eqLo=0, eqMid=0, eqHi=0, opto=0.5, maxAmt=0)
    base.update({k: float(x) for k, x in kw.items()})
    base.update(chain(**fx))
    DNB.append((name[:30], base))
MONO = dict(Stereo="Mono Bass", Space="Dry", Opto="Smooth")
# techstep: two saws a fifth or fourth apart, LFO on the cutoff, tape style saturation, mono
dnb("Techstep Fifth", dict(EQ="Tight Low End", OTT="Light", Maximizer="Big", **MONO), bSemi=7, bTune=0.05, mixA=0.85, mixB=0.8, cutoff=760, reso=0.45, fenvAmt=0.3, fD=0.35, lfoHz=4.35, mLfo=0.4, mLfoR=1, dest4=1, drive=0.5, xstr=0.7, fuzz=0.45, eqMid=3)
dnb("Techstep Fourth", dict(EQ="Mud Cut", Maximizer="Big", **MONO), bSemi=5, bSaw=0, bPulse=1, bPW=0.45, mixA=0.85, mixB=0.7, colPos=0, cutoff=640, reso=0.35, fenvAmt=0.45, fD=0.3, fS=0.2, mLfo=0, drive=0.3, xstr=0.55, fuzz=0.1, eqMid=2)
dnb("Warp Machine", dict(EQ="Mud Cut", Maximizer="Big", **MONO), bSemi=7, bSaw=0, bPulse=1, bPW=0.35, cutoff=520, reso=0.5, fenvAmt=0.35, lfoHz=8.7, lfoTri=0, lfoPulse=1, mLfo=0.5, mLfoR=1, dest4=1, drive=0.4, xstr=0.7, fuzz=0.5)
dnb("Hydraulic Roll", dict(EQ="Tight Low End", Maximizer="Glue", **MONO), bSemi=7, bTune=0.1, uniVoices=1, uniDetune=0.25, glide=0.25, cutoff=300, reso=0.4, fenvAmt=0.6, fD=0.6, fS=0.3, aD=0.6, aS=0.9, drive=0.3, xstr=0.6, fuzz=0.25)
# neuro: detuned saws, B into the filter (filter FM), LFO on filter and B pitch, drive before and after, OTT
dnb("Noisia Growl", dict(EQ="Dynamic Tame", OTT="Full", Stereo="Wide", Space="Dry", Opto="Smooth"), bTune=0.35, cutoff=480, reso=0.48, fenvAmt=0.3, fD=0.5, lfoHz=4.35, lfoTri=0, lfoSaw=1, mLfo=0.55, mLfoR=1, mOscB=0.12, mOscBR=1, dest2=1, dest4=1, xstr=0.55, drive=0.25, fuzz=0.55)
dnb("Neuro Talk", dict(EQ="Presence", OTT="Classic", Stereo="Narrow", Space="Dry", Opto="Smooth"), fMode=2, bTune=0.5, cutoff=700, reso=0.55, fenvAmt=0.2, lfoHz=5.8, mLfo=0.7, mLfoR=1, dest4=1, drive=0.3, xstr=0.55, fuzz=0.5)
dnb("Neuro HP Naughty", dict(EQ="Scoop", OTT="Classic", Stereo="Wide", Space="Dry", Opto="Smooth"), fMode=1, mixSub=0.0, bTune=0.3, cutoff=650, reso=0.45, fenvAmt=0.25, lfoHz=4.35, mLfo=0.5, mLfoR=1, dest4=1, drive=0.35, xstr=0.6, fuzz=0.6)
dnb("Neuro Rolling", dict(EQ="Dynamic Tame", OTT="Light", Stereo="Wide", Space="Dry", Opto="Smooth"), bTune=0.3, cutoff=560, reso=0.45, fenvAmt=0.35, fD=0.3, fS=0.15, aD=0.35, aS=0.6, lfoHz=2.9, mLfo=0.35, mLfoR=1, dest4=1, xstr=0.5, drive=0.2, fuzz=0.45)
dnb("Neuro Vowel", dict(EQ="Presence", OTT="Full", Stereo="Narrow", Space="Dry", Opto="Smooth"), fMode=2, bSemi=7, cutoff=900, reso=0.6, fenvAmt=0.15, lfoHz=3.2, mLfo=0.8, mLfoR=1, mOscB=0.1, mOscBR=1, dest4=1, drive=0.3, xstr=0.55, fuzz=0.5)
dnb("Neuro Screamer", dict(EQ="Presence", OTT="Full", Stereo="Wide", Space="Dry", Opto="Squash"), bTune=0.6, cutoff=800, reso=0.78, fenvAmt=0.4, fD=0.3, lfoHz=5.8, lfoTri=0, lfoSaw=1, mLfo=0.45, mLfoR=1, dest4=1, drive=0.4, xstr=0.65, fuzz=0.7)
# reese and Bad Company style: two saws fighting, unstable filter, amp like crunch
dnb("Bad Boy Reese", dict(EQ="Presence", Stereo="Wide", Space="Dry", Opto="Smooth"), bTune=0.35, uniVoices=1, uniDetune=0.35, cutoff=420, reso=0.3, fenvAmt=0.15, lfoHz=0.7, mLfo=0.5, mLfoR=1, dest4=1, drive=0.3, xstr=0.65, fuzz=0.5, eqMid=2)
dnb("Ripping Canvas", dict(EQ="Presence", Stereo="Wide", Space="Dry", Opto="Squash"), bTune=0.4, uniVoices=1, uniDetune=0.4, cutoff=520, reso=0.35, fenvAmt=0.25, lfoHz=1.3, mLfo=0.4, mLfoR=1, dest4=1, drive=0.45, xstr=0.7, fuzz=0.65, eqMid=3)
dnb("Warehouse Reese", dict(EQ="Mono Low End", Stereo="Wide", Space="Dry", Opto="Smooth"), bTune=0.3, uniVoices=1, uniDetune=0.5, mixSub=0.45, cutoff=360, reso=0.25, fenvAmt=0.1, lfoHz=0.5, mLfo=0.35, mLfoR=1, dest4=1, drive=0.2, xstr=0.5, fuzz=0.35)
# liquid: warm and gentle, clean sub under a soft reese
dnb("Liquid Roller", dict(EQ="Mud Cut", Stereo="Wide", Space="Plate", Opto="Smooth"), bTri=1, bSaw=0, bTune=0.15, mixB=0.5, mixSub=0.55, cutoff=360, reso=0.15, fenvAmt=0.22, fD=0.45, fS=0.2, aD=0.5, aS=0.8, drive=0.0, xstr=0.3, fuzz=0.0)
dnb("Liquid Reese", dict(EQ="Mud Cut", Stereo="Wide", Space="Plate", Opto="Smooth"), bTune=0.25, uniVoices=1, uniDetune=0.3, mixSub=0.5, cutoff=340, reso=0.2, fenvAmt=0.15, lfoHz=0.4, mLfo=0.25, mLfoR=1, dest4=1, drive=0.0, xstr=0.35, fuzz=0.05)
dnb("Soft Sub Roller", dict(EQ="Tight Low End", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), bSaw=0, bTri=1, mixA=0.5, mixB=0.4, mixSub=0.7, cutoff=300, reso=0.1, fenvAmt=0.25, fD=0.35, fS=0.1, aD=0.4, aS=0.7, drive=0.0, xstr=0.25, fuzz=0.0)
# Pendulum: unison saw wall, square fill, pitch punch at the start, heavy distortion, wide
dnb("Pendulum Wall", dict(EQ="Side Air", Stereo="Extra Wide", Space="Dry", Dance_master="Loud", Opto="Squash"), bSaw=0, bPulse=1, bPW=0.5, uniVoices=4, uniDetune=0.7, cutoff=1400, reso=0.2, fenvAmt=0.35, fD=0.2, fS=0.3, mEnv=0.8, mEnvR=1, dest0=1, dest2=1, drive=0.5, xstr=0.7, fuzz=0.8, eqHi=2)
dnb("Propane Fuzz", dict(EQ="Presence", Stereo="Wide", Space="Dry", Dance_master="Loud", Opto="Squash"), bTune=0.1, uniVoices=3, uniDetune=0.6, cutoff=1100, reso=0.25, fenvAmt=0.3, fD=0.25, lfoHz=5.8, lfoTri=0, lfoSaw=1, mLfo=0.2, mLfoR=1, dest4=1, drive=0.55, xstr=0.7, fuzz=0.85, eqHi=3)
dnb("Pendulum Chug", dict(EQ="Presence", Stereo="Wide", Space="Dry", Dance_master="Loud", Opto="Fast Grab"), bSaw=0, bPulse=1, bPW=0.4, uniVoices=3, uniDetune=0.5, cutoff=900, reso=0.3, fenvAmt=0.5, fD=0.12, fS=0.1, aD=0.22, aS=0.5, aR=0.08, mEnv=0.7, mEnvR=1, dest0=1, dest2=1, drive=0.5, xstr=0.65, fuzz=0.8)
dnb("Cyber Guitar", dict(EQ="Presence", Stereo="Extra Wide", Space="Room", Dance_master="Loud", Opto="Squash"), aSaw=1, bSaw=1, bSemi=12, uniVoices=4, uniDetune=0.6, cutoff=1800, reso=0.3, fenvAmt=0.3, fD=0.3, lfoHz=8.7, mLfo=0.12, mLfoR=1, dest4=1, drive=0.6, xstr=0.75, fuzz=0.9, eqHi=3)
# screech: hard sync swept by the envelope, high resonance, high pass, hard clip
dnb("Screech Sync", dict(EQ="Presence", OTT="Full", Stereo="Wide", Space="Dry", Opto="Squash"), aSync=1, bTune=0.0, mixA=0.8, mixB=0.6, mixSub=0.1, cutoff=1500, reso=0.45, fenvAmt=0.25, fD=0.4, fS=0.2, mEnv=1.0, mEnvR=1, dest2=1, drive=0.4, xstr=0.65, fuzz=0.7)
dnb("Metal Pipe", dict(EQ="Presence", OTT="Classic", Stereo="Wide", Space="Dry", Opto="Squash"), fMode=1, aSync=1, mixA=0.9, mixB=0.9, mixSub=0.0, cutoff=2600, reso=0.65, fenvAmt=0.3, fD=0.35, mEnv=0.6, mEnvR=1, dest2=1, lfoHz=11.6, mLfo=0.2, mLfoR=1, dest4=1, drive=0.4, xstr=0.7, fuzz=0.75)
dnb("Techy Tearer", dict(EQ="Presence", OTT="Full", Stereo="Narrow", Space="Dry", Dance_master="Loud", Opto="Squash"), fMode=1, mixSub=0.0, bTune=0.5, cutoff=1200, reso=0.55, fenvAmt=0.3, fD=0.3, lfoHz=5.8, mLfo=0.4, mLfoR=1, dest4=1, drive=0.5, xstr=0.7, fuzz=0.85, eqHi=2)
dnb("Laser Screech", dict(EQ="Presence", OTT="Classic", Stereo="Wide", Space="Dry", Opto="Squash"), fMode=1, aSync=1, mixA=0.9, mixB=0.9, mixSub=0.0, cutoff=2600, reso=0.55, fenvAmt=0.4, fD=0.2, fS=0.0, aD=0.25, aS=0.4, mEnv=1.0, mEnvR=1, dest0=1, dest2=1, drive=0.4, xstr=0.65, fuzz=0.8)
# foghorn: hollow square with a bandpass, short pitch knock, narrow and mono in the mids
dnb("Foghorn", dict(EQ="Presence", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), fMode=2, aSaw=0, aPulse=1, aPW=0.4, bSaw=1, mixA=0.8, mixB=0.3, mixSub=0.2, cutoff=520, reso=0.4, fenvAmt=0.2, fD=0.3, fS=0.35, mEnv=0.25, mEnvR=1, dest0=1, dest2=1, drive=0.3, xstr=0.6, fuzz=0.4)
dnb("Foghorn Hollow", dict(EQ="Presence", Stereo="Narrow", Space="Dry", Opto="Smooth"), fMode=2, aSaw=0, aPulse=1, aPW=0.35, bSaw=0, bPulse=1, bPW=0.6, bSemi=12, mixA=0.8, mixB=0.4, mixSub=0.15, cutoff=420, reso=0.5, fenvAmt=0.15, mEnv=0.2, mEnvR=1, dest0=1, dest2=1, drive=0.3, xstr=0.65, fuzz=0.45)
# laser / zap
dnb("Laser Zap Bass", dict(EQ="Presence", Stereo="Wide", Space="Dry", Dance_master="Loud", Opto="Fast Grab"), fMode=1, bSemi=7, cutoff=700, reso=0.35, fenvAmt=0.5, fD=0.12, fS=0.0, aD=0.2, aS=0.3, aR=0.08, mEnv=1.0, mEnvR=1, dest0=1, dest2=1, drive=0.4, xstr=0.65, fuzz=0.7)
dnb("Pew Roll", dict(EQ="Presence", Stereo="Wide", Space="Dry", Opto="Fast Grab"), cutoff=900, reso=0.4, fenvAmt=0.45, fD=0.15, fS=0.0, aD=0.25, aS=0.35, aR=0.08, mEnv=0.9, mEnvR=1, dest0=1, dest2=1, drive=0.3, xstr=0.6, fuzz=0.6)
# jump up
dnb("Jump Up Wub", dict(EQ="Tight Low End", Stereo="Narrow", Space="Dry", Dance_master="Bass Safe", Opto="Smooth"), aSaw=0, aPulse=1, aPW=0.5, bSaw=1, bTune=0.1, cutoff=520, reso=0.4, fenvAmt=0.3, lfoHz=5.8, lfoTri=0, lfoSaw=1, mLfo=0.6, mLfoR=1, dest4=1, drive=0.3, xstr=0.6, fuzz=0.5)
dnb("Jump Up Stab", dict(EQ="Presence", Stereo="Narrow", Space="Dry", Dance_master="Bass Safe", Opto="Fast Grab"), aSaw=0, aPulse=1, aPW=0.45, bSaw=1, bSemi=12, cutoff=700, reso=0.35, fenvAmt=0.6, fD=0.2, fS=0.0, aD=0.25, aS=0.2, aR=0.08, drive=0.3, xstr=0.6, fuzz=0.55)
# old school Pro-One: both oscillators loud into the filter, pre filter saturation, mono, low mid boost
dnb("Old School Pro-1", dict(EQ="Classic", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), bTune=0.15, mixA=1.0, mixB=1.0, mixSub=0.2, colPos=0, cutoff=600, reso=0.4, fenvAmt=0.35, fD=0.35, fS=0.2, kbd=0.5, drive=0.4, xstr=0.65, fuzz=0.3, eqMid=2)
dnb("Pro-1 Warp", dict(EQ="Mud Cut", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), bSemi=7, mixA=1.0, mixB=1.0, mixSub=0.2, colPos=0, cutoff=500, reso=0.45, fenvAmt=0.3, fD=0.4, lfoHz=4.35, mLfo=0.4, mLfoR=1, dest4=1, drive=0.4, xstr=0.7, fuzz=0.35)
dnb("Pro-1 Filter FM", dict(EQ="Classic", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), bTune=0.0, bSaw=0, bTri=1, mixA=1.0, mixB=0.6, mixSub=0.2, colPos=0, cutoff=620, reso=0.5, fenvAmt=0.3, fD=0.35, mOscB=0.18, mOscBR=1, dest4=1, drive=0.4, xstr=0.65, fuzz=0.35)
# jungle sub, deewp, neuro reese, crossbreed
dnb("808 Roller Glide", dict(EQ="Tight Low End", Stereo="Mono Bass", Space="Dry", Opto="Smooth"), aSaw=0, bSaw=0, aPulse=0, bTri=1, aOct=0, bOct=0, mixA=0.0, mixB=0.9, mixSub=0.6, cutoff=260, reso=0.1, fenvAmt=0.05, fD=0.3, fS=0.5, aD=0.5, aS=0.9, glide=0.34, glideAuto=0, drive=0.15, xstr=0.4, fuzz=0.0, colPos=1)
dnb("Deewp Donk Sub", dict(EQ="Tight Low End", Stereo="Mono Bass", Space="Dry", Opto="Squash"), aSaw=0, bSaw=0, bTri=1, mixA=0.0, mixB=0.8, mixSub=0.7, cutoff=420, reso=0.15, fenvAmt=0.15, fD=0.3, fS=0.2, aD=0.5, aS=0.85, aR=0.1, mEnv=1.0, mEnvR=1, dest0=1, dest2=1, drive=0.3, xstr=0.5, fuzz=0.1, colPos=1)
dnb("Neuro Reese Matrix", dict(EQ="Dynamic Tame", OTT="Light", Crush="Subtle 12 bit", Stereo="Wide", Space="Dry", Opto="Smooth"), bTune=0.4, uniVoices=1, uniDetune=0.4, cutoff=520, reso=0.45, fenvAmt=0.25, lfoHz=0.9, lfoTri=1, mLfo=0.7, mLfoR=1, mOscB=0.2, mOscBR=1, dest2=1, dest4=1, drive=0.2, xstr=0.45, fuzz=0.35)
dnb("Crossbreed Industrial", dict(EQ="Scoop", Crush="Crunch 6 bit", Stereo="Narrow", Space="Dark Cave", Dance_master="Loud", Opto="Squash"), aSaw=0, aPulse=1, aPW=0.5, bSaw=1, cutoff=900, reso=0.5, fenvAmt=0.35, fD=0.2, fS=0.1, drive=0.8, xstr=0.8, fuzz=0.8)
# call and response sequences at 174 BPM: a rolling hook with an answer on the snare steps (5 and 13 of 16), as in the Noisia / Mefjus style
PATS["DnB Hook"]   = ("0 0 0 - 12 - - 12 0 0 - 0 - 7 - 12", 2, 0.7, 0.5)
PATS["DnB Two Step"] = ("0 - - 0 - - 0 - 0 - - - 7 - 10 -", 2, 0.6, 0.5)
PATS["DnB Stab"]   = ("0 - 0 - 12a - - - 0 - 0 - 24a - 12a -", 2, 0.35, 0.5)
PATS["DnB Drop"]   = ("0 - - - - - - - -12 - - - 0 - 7 -", 2, 0.95, 0.5)
PATS["DnB Roll"]   = ("0 0 12 0 0 0 7 0 0 0 12 0 0 5 7 10", 2, 0.75, 0.5)
DNBV = dict(DNB)
DNBSEQ = [("Techstep Fifth", "DnB Hook", "Techstep Hook"), ("Warp Machine", "DnB Two Step", "Warp Two Step"), ("Noisia Growl", "DnB Roll", "Noisia Roll"),
          ("Neuro Rolling", "DnB Hook", "Neuro Call"), ("Laser Zap Bass", "DnB Stab", "Laser Answer"), ("Pew Roll", "DnB Stab", "Pew Stab Answer"),
          ("Foghorn", "DnB Drop", "Foghorn Drop"), ("Pendulum Chug", "DnB Roll", "Pendulum Roll"), ("Jump Up Wub", "DnB Two Step", "Jump Up Two Step"),
          ("Old School Pro-1", "DnB Hook", "Pro-1 Hook")]
m = mark("D")
for n, vals in DNB: addp(n, "DnB", vals)
for n, label, nm in DNBSEQ:
    o, sp = seqvals(DNBV[n], label, 5, bpm=174)
    addp(nm, "Seq", o, sp)
m()

# ------------------------------------------------------------------ checks
for b in "ABCD":
    print("bank", b, len(bank[b]) + (20 if b == "A" else 0), file=sys.stderr)
assert len(bank["A"]) == 108, len(bank["A"])
assert len(bank["B"]) == 128, len(bank["B"])
assert len(bank["C"]) == 128, len(bank["C"])
assert 0 < len(bank["D"]) <= 128, len(bank["D"])
names = [p[0] for p in PRESETS] + list(CORE)
dups = {n for n in names if names.count(n) > 1}
assert not dups, dups
for p in PRESETS:
    assert len(p[0]) <= 30, p[0]
    for k in p[2]:
        assert k in DEF, k

# ------------------------------------------------------------------ output
def fmt(x):
    s = "%.4f" % x
    s = s.rstrip("0").rstrip(".") if "." in s else s
    return s + "f" if "." in s else s + ".0f"

def entry(name, cat, vals, seq):
    vals = dict(vals)
    if name in LEVELS and name not in CORE: vals["vol"] = LEVELS[name]
    items = []
    for k, x in vals.items():
        if k not in DEF: raise KeyError(k)
        t, d, lo, hi = DEF[k]
        x = min(hi, max(lo, x))
        if abs(x - d) < 1e-6: continue
        items.append('{"%s", %s}' % (k, fmt(x)))
    return '        { "%s", "%s", { %s }, "%s" },\n' % (name, cat, ", ".join(items), seq)

out = ['// GENERATED by tools/gen_presets.py. Do not edit by hand.\n// Banks A (after the 20 hand made sounds), B, C and D (drum and bass) of the factory sounds.\n#pragma once\n#include "PresetTypes.h"\n\nnamespace purrone\n{\ninline const std::vector<FactoryPreset>& generatedPresets()\n{\n    static const std::vector<FactoryPreset> list = {\n']
for b in "ABCD":
    out.append("        // ---- bank %s ----\n" % b)
    for i in bank[b]:
        out.append(entry(*PRESETS[i]))
out.append("    };\n    return list;\n}\n} // namespace purrone\n")
open(os.path.join(ROOT, "src", "PresetsGen.h"), "w").write("".join(out))

# fx module presets
fo = ['// GENERATED by tools/gen_presets.py. Do not edit by hand.\n// The dropdown presets of each FX module. A module preset sets only that module\'s parameters.\n#pragma once\n#include <utility>\n#include <vector>\n\nnamespace purrone\n{\nstruct FxModulePreset { const char* name; std::vector<std::pair<const char*, float>> values; };\nstruct FxModule { const char* module; std::vector<FxModulePreset> presets; };\n\ninline const std::vector<FxModule>& fxModules()\n{\n    static const std::vector<FxModule> list = {\n']
# full parameter list per module so a preset also resets what it does not mention
MODKEYS = {
 "Colour": ["drive", "xstr", "fuzz", "colPos"],
 "Crush": ["crushOn", "crushBits", "crushRate", "crushMix"],
 "EQ": ["eqLo", "eqLoF", "eqG2", "eqF2", "eqQ2", "eqMid", "eqMidF", "eqMidQ", "eqG4", "eqF4", "eqQ4", "eqHi", "eqHiF"] + ["eq%s%d" % (a, i) for i in range(1, 6) for a in "DTM"],
 "Space": ["verbMix", "verbSize", "verbHP", "spPre", "spDamp", "spWid", "spDuck", "fxOnSpace"],
 "Opto": ["opto", "optoSpd"],
 "OTT": ["ottOn", "ottAmt", "ottDepth", "ottTime", "ottLo", "ottMid", "ottHi"],
 "Dance master": ["dmOn", "dmMix", "dmDrive", "dmClip", "dmLow", "dmMid", "dmHigh"],
 "Maximizer": ["maxAmt"],
 "Stereo": ["stOn", "stW", "stWLo", "stWHi", "stMono", "stMid", "stSide", "stMidDrv", "stSideDrv"],
}
for module, plist in FX.items():
    fo.append('        { "%s", {\n' % module)
    for name, vals in plist:
        items = []
        for k in MODKEYS[module]:
            x = vals.get(k, DEF[k][1])
            items.append('{"%s", %s}' % (k, fmt(x)))
        fo.append('            { "%s", { %s } },\n' % (name, ", ".join(items)))
    fo.append("        } },\n")
fo.append("    };\n    return list;\n}\n} // namespace purrone\n")
open(os.path.join(ROOT, "src", "FxPresets.h"), "w").write("".join(fo))
print("ok: %d generated presets" % len(PRESETS), file=sys.stderr)
