// Interface widgets that follow the approved mockup (docs/mockup/purrone-mockup.html).
// Everything is drawn in a fixed 1700 x 828 space; the editor scales it to the window.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Layout.h"
#include "../Parameters.h"

namespace purrone::ui
{
using namespace juce;

namespace col
{
    inline const Colour ground { 0xff0b0b0d }, panel { 0xff15161a }, panel2 { 0xff1c1d22 }, line { 0xff2b2d34 }, ink { 0xfff4f5f7 }, inkDim { 0xffa9adb8 },
                        knob { 0xff23252c }, green { 0xff02EBBA }, yellow { 0xffFFD100 }, orange { 0xffFF9000 }, pink { 0xffFF1383 }, blue { 0xff6CC9F5 },
                        field { 0xff101115 }, selected { 0xff2c2f38 }, black { 0xff0a0a0c };
}

inline Rectangle<int> rc (const layout::R& r) { return { r.x, r.y, r.w, r.h }; }

// Condensed look for labels (the mockup uses Barlow Condensed; a horizontally squeezed system sans is the portable stand-in).
inline Font labelFont (float h, bool bold = false, float tracking = 0.04f)
{
    return Font (FontOptions (h).withName (Font::getDefaultSansSerifFontName()).withStyle (bold ? "Bold" : "Regular").withHorizontalScale (0.86f).withKerningFactor (tracking));
}
inline Font monoFont (float h) { return Font (FontOptions (h).withName (Font::getDefaultMonospacedFontName()).withStyle ("Regular")); }

// Shared switch for the info bubbles.
struct Bubbles { bool on = true; };

// Text for a knob value, same wording as the mockup.
inline String formatValue (char f, float v)
{
    auto sgn = [] (float x, int d = 1) { return String (x > 0 ? "+" : "") + String (x, d); };
    switch (f)
    {
        case 'S': return sgn (v);
        case 'C': return (v >= 1000.0f ? String (v / 1000.0f, 1) + "k" : String (roundToInt (v))) + "Hz";
        case 'E': return sgn (v * 6.0f) + " oct";
        case 'T': { const float s = 0.002f * std::pow (3000.0f, v); return s < 1.0f ? String (roundToInt (s * 1000.0f)) + "ms" : String (s, 1) + "s"; }
        case 'L': return String (v, 1) + "Hz";
        case 'G': return v < 0.01f ? String ("off") : String (0.005f * std::pow (800.0f, v), 2) + "s";
        case 'D': return sgn (v) + "dB";
        case 'V': return String (v, 1) + "s";
        case 'H': return String (roundToInt (v)) + "Hz";
        case 'B': return (v > 0.0f ? String (20.0f * std::log10 (v), 1) : String ("-inf")) + "dB";
        default:  return String (roundToInt (v * 100.0f)) + "%";
    }
}

//==============================================================================
class Knob : public Component, public SettableTooltipClient
{
public:
    Knob (RangedAudioParameter& p, const layout::Knob& d, Bubbles& b) : param (p), def (d), bubbles (b),
        attach (p, [this] (float) { repaint(); })
    {
        setBounds (rc (def.wrap).expanded (14, 0));   // wider than the knob so long labels are not clipped
        attach.sendInitialUpdate();
        setTooltip (String (def.label) + "\n" + def.tip);
    }
    String getTooltip() override { return bubbles.on && String (def.tip).isNotEmpty() ? SettableTooltipClient::getTooltip() : String(); }

    void paint (Graphics& g) override
    {
        const float v = param.convertFrom0to1 (param.getValue()), n = param.getValue();
        g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.06f));
        g.drawText (def.label, 0, 0, getWidth(), 13, Justification::centred, false);

        const float cx = (float) getWidth() * 0.5f, cy = 14.0f + 22.0f, r = 22.0f;
        const float a0 = degreesToRadians (-135.0f), a1 = degreesToRadians (135.0f), av = a0 + n * (a1 - a0);
        auto arc = [&] (float from, float to, Colour c)
        {
            Path p; p.addCentredArc (cx, cy, r, r, 0.0f, from, to, true);
            g.setColour (c); g.strokePath (p, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        };
        arc (a0, a1, Colour (0xff33353d));
        if (n > 0.002f) arc (a0, av, col::green);

        const Rectangle<float> body (cx - 22.0f, cy - 22.0f, 44.0f, 44.0f);
        g.setGradientFill (ColourGradient (Colour (0xff3a3d47), cx - 22.0f * 0.3f, cy - 22.0f * 0.4f, Colour (0xff14151a), cx + 20.0f, cy + 22.0f, true));
        g.fillEllipse (body);
        g.setColour (Colours::black); g.drawEllipse (body, 1.0f);
        g.setColour (Colours::white.withAlpha (0.10f)); g.drawEllipse (body.reduced (1.5f), 1.0f);
        Path t; t.addRoundedRectangle (-1.0f, -18.0f, 2.0f, 14.0f, 1.0f);
        g.setColour (col::ink); g.fillPath (t, AffineTransform::rotation (av, 0.0f, 0.0f).translated (cx, cy));
        if (hasKeyboardFocus (false)) { g.setColour (col::blue); g.drawEllipse (body.expanded (3.0f), 2.0f); }

        g.setColour (col::ink); g.setFont (monoFont (11.0f));
        g.drawText (formatValue (def.fmt, v), 0, 60, getWidth(), 14, Justification::centred, false);
    }

    void mouseDown (const MouseEvent&) override { dragStart = param.getValue(); attach.beginGesture(); }
    void mouseDrag (const MouseEvent& e) override
    {
        const float dy = (float) -e.getDistanceFromDragStartY();
        const float n = jlimit (0.0f, 1.0f, dragStart + dy / (e.mods.isShiftDown() ? 700.0f : 180.0f));
        attach.setValueAsPartOfGesture (param.convertFrom0to1 (n));
    }
    void mouseUp (const MouseEvent&) override { attach.endGesture(); }
    void mouseDoubleClick (const MouseEvent&) override { attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override
    {
        const float step = (w.deltaY > 0 ? 0.03f : -0.03f) * (w.isReversed ? -1.0f : 1.0f);
        if (w.deltaY != 0.0f) attach.setValueAsCompleteGesture (param.convertFrom0to1 (jlimit (0.0f, 1.0f, param.getValue() + step)));
    }
    bool keyPressed (const KeyPress& k) override
    {
        const float st = k.getModifiers().isShiftDown() ? 0.01f : 0.05f; float n = param.getValue();
        if (k == KeyPress::upKey || k == KeyPress::rightKey || k.getKeyCode() == KeyPress::upKey || k.getKeyCode() == KeyPress::rightKey) n += st;
        else if (k.getKeyCode() == KeyPress::downKey || k.getKeyCode() == KeyPress::leftKey) n -= st; else return false;
        attach.setValueAsCompleteGesture (param.convertFrom0to1 (jlimit (0.0f, 1.0f, n))); return true;
    }
    bool hitTest (int x, int y) override { return x >= 14 && y >= 0 && x < getWidth() - 14 && y < getHeight(); }
private:
    RangedAudioParameter& param; const layout::Knob& def; Bubbles& bubbles; ParameterAttachment attach; float dragStart = 0.0f;
};

//==============================================================================
class LedToggle : public Component, public SettableTooltipClient
{
public:
    LedToggle (RangedAudioParameter& p, const layout::Tog& d, Bubbles& b) : param (p), def (d), bubbles (b), attach (p, [this] (float) { repaint(); })
    {
        setBounds (rc (def.r)); attach.sendInitialUpdate(); setMouseCursor (MouseCursor::PointingHandCursor);
        if (String (def.tip).isNotEmpty()) setTooltip (String (def.label) + "\n" + def.tip);
    }
    String getTooltip() override { return bubbles.on ? SettableTooltipClient::getTooltip() : String(); }
    void paint (Graphics& g) override
    {
        const bool on = param.getValue() > 0.5f; const auto r = getLocalBounds().toFloat();
        g.setColour (col::field); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        const Colour c (def.colour); const float cy = r.getCentreY();
        const Rectangle<float> led (r.getX() + 9.0f, cy - 4.5f, 9.0f, 9.0f);
        if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (led.expanded (3.0f)); g.setColour (c); } else g.setColour (Colour (0xff2a2c33));
        g.fillEllipse (led); g.setColour (Colours::black.withAlpha (0.5f)); g.drawEllipse (led, 1.0f);
        g.setColour (on ? col::ink : col::inkDim); g.setFont (labelFont (13.0f, true, 0.08f));
        g.drawFittedText (def.label, Rectangle<int> (26, 0, getWidth() - 30, getHeight()), Justification::centredLeft, 2, 0.8f);
    }
    void mouseDown (const MouseEvent&) override { attach.setValueAsCompleteGesture (param.getValue() > 0.5f ? 0.0f : 1.0f); }
private:
    RangedAudioParameter& param; const layout::Tog& def; Bubbles& bubbles; ParameterAttachment attach;
};

//==============================================================================
class Segments : public Component, public SettableTooltipClient
{
public:
    Segments (RangedAudioParameter& p, const layout::Seg& d, Bubbles& b) : param (p), def (d), bubbles (b), attach (p, [this] (float) { repaint(); })
    {
        setBounds (rc (def.r)); attach.sendInitialUpdate(); setMouseCursor (MouseCursor::PointingHandCursor);
        if (String (def.tip).isNotEmpty()) setTooltip (def.tip); else if (def.labelFirst) setTooltip ("Click the name to cycle Off, Direct, Wheel");
    }
    String getTooltip() override { return bubbles.on ? SettableTooltipClient::getTooltip() : String(); }
    int current() const { return roundToInt (param.convertFrom0to1 (param.getValue())); }
    Rectangle<int> btn (int i) const { auto r = rc (layout::segBtns[def.first + i].r); return r.translated (-def.r.x, -def.r.y); }
    void paint (Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        { Graphics::ScopedSaveState s (g); Path clip; clip.addRoundedRectangle (all, 3.0f); g.reduceClipRegion (clip);
          g.fillAll (col::field);
          for (int i = 0; i < def.n; ++i)
          {
              const bool label = def.labelFirst && i == 0, sel = ! label && (i - (def.labelFirst ? 1 : 0)) == current();
              const auto b = btn (i);
              if (sel) { g.setColour (col::selected); g.fillRect (b); }
              g.setColour (sel ? col::ink : col::inkDim); g.setFont (labelFont (13.0f, true, 0.08f));
              g.drawText (layout::segBtns[def.first + i].text, b, Justification::centred, false);
          } }
        g.setColour (col::line); g.drawRoundedRectangle (all.reduced (0.5f), 3.0f, 1.0f);
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < def.n; ++i)
            if (btn (i).contains (e.getPosition()))
            {
                const int off = def.labelFirst ? 1 : 0; const int count = def.n - off;
                const int next = (def.labelFirst && i == 0) ? (current() + 1) % count : i - off;
                attach.setValueAsCompleteGesture (param.convertTo0to1 ((float) next)); return;
            }
    }
private:
    RangedAudioParameter& param; const layout::Seg& def; Bubbles& bubbles; ParameterAttachment attach;
};

//==============================================================================
// Small graph that redraws when its parameters change: filter response, two envelopes, EQ curve.
class Display : public Component
{
public:
    enum Kind { filter, fenv, aenv, eq };
    Display (AudioProcessorValueTreeState& s, Kind k, const layout::R& r) : apvts (s), kind (k) { setBounds (rc (r)); setInterceptsMouseClicks (false, false); }
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float signature() const
    {
        switch (kind)
        {
            case filter: return v (ids::cutoff) * 1.0f + v (ids::reso) * 7919.0f + v (ids::fMode) * 104729.0f;
            case fenv:   return v (ids::fA) + v (ids::fD) * 3.0f + v (ids::fS) * 5.0f + v (ids::fR) * 7.0f;
            case aenv:   return v (ids::aA) + v (ids::aD) * 3.0f + v (ids::aS) * 5.0f + v (ids::aR) * 7.0f;
            default:     return v (ids::eqLo) + v (ids::eqMid) * 31.0f + v (ids::eqHi) * 977.0f;
        }
    }
    void tick() { const float s = signature(); if (s != last) { last = s; repaint(); } }

    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        g.setColour (col::black); g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);
        g.setColour (col::line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
        g.setColour (Colour (0xff1d1f26));
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (i * h / 4), 0.0f, w);
        g.setFont (monoFont (9.0f));
        auto xFor = [&] (float hz) { return std::log (hz / 20.0f) / std::log (1000.0f) * w; };
        auto freqGrid = [&] (std::initializer_list<float> list)
        {
            for (float f : list) { const float x = xFor (f); g.setColour (Colour (0xff1d1f26)); g.drawVerticalLine ((int) x, 0.0f, h);
                g.setColour (Colour (0xff6f7480)); g.drawText (f >= 1000 ? String (f / 1000, 0) + "k" : String ((int) f), (int) x + 2, (int) h - 12, 30, 10, Justification::left, false); }
        };
        if (kind == filter) paintFilter (g, w, h, freqGrid);
        else if (kind == eq) paintEq (g, w, h, freqGrid);
        else paintEnv (g, w, h);
    }
private:
    AudioProcessorValueTreeState& apvts; Kind kind; float last = -1.0e9f;

    static float st (float w, float q) { const float re = 1.0f - w * w, im = w / q; return 1.0f / std::sqrt (re * re + im * im); }
    template <typename F> void paintFilter (Graphics& g, float w, float h, F grid)
    {
        grid ({ 100.0f, 1000.0f, 10000.0f });
        const float f0 = v (ids::cutoff), q1 = 0.7f + v (ids::reso) * 14.0f, q2 = jmax (0.5f, q1 * 0.35f);
        Path p; const int n = 160;
        for (int i = 0; i < n; ++i)
        {
            const float fr = 20.0f * std::pow (1000.0f, (float) i / (n - 1)), ww = v (ids::fMode) > 0.5f ? f0 / fr : fr / f0, m = st (ww, q1) * st (ww, q2);
            const float y = h - jlimit (0.0f, 1.0f, (20.0f * std::log10 (m + 1.0e-6f) + 40.0f) / 64.0f) * h, x = (float) i / (n - 1) * w;
            if (i) p.lineTo (x, y); else p.startNewSubPath (x, y);
        }
        Path fill (p); fill.lineTo (w, h); fill.lineTo (0, h); fill.closeSubPath();
        g.setColour (col::green.withAlpha (0.12f)); g.fillPath (fill);
        g.setColour (col::green); g.strokePath (p, PathStrokeType (2.0f));
        g.setColour (col::inkDim); g.setFont (monoFont (9.0f)); g.drawText ("+24 dB", 4, 2, 50, 10, Justification::left, false); g.drawText ("-40", 4, (int) h - 24, 30, 10, Justification::left, false);
    }
    template <typename F> void paintEq (Graphics& g, float w, float h, F grid)
    {
        g.setColour (col::line); g.drawHorizontalLine ((int) (h / 2), 0.0f, w);
        grid ({ 100.0f, 1000.0f, 6000.0f });
        Path p;
        for (int i = 0; i < 200; ++i)
        {
            const float fr = 20.0f * std::pow (1000.0f, (float) i / 199.0f);
            const float lo = v (ids::eqLo) / (1.0f + std::pow (fr / 100.0f, 2.0f)), hi = v (ids::eqHi) / (1.0f + std::pow (6000.0f / fr, 2.0f));
            const float bell = v (ids::eqMid) * std::exp (-std::pow (std::log (fr / 1000.0f) / 0.7f, 2.0f));
            const float db = lo + hi + bell, y = h / 2 - db / 14.0f * (h / 2 - 4.0f), x = (float) i / 199.0f * w;
            if (i) p.lineTo (x, y); else p.startNewSubPath (x, y);
        }
        g.setColour (col::yellow); g.strokePath (p, PathStrokeType (2.0f));
        g.setColour (col::inkDim); g.setFont (monoFont (9.0f)); g.drawText ("+12", 4, 1, 30, 10, Justification::left, false); g.drawText ("-12", 4, (int) h - 22, 30, 10, Justification::left, false);
    }
    void paintEnv (Graphics& g, float w, float h)
    {
        const bool amp = kind == aenv; const Colour c = amp ? col::orange : col::blue;
        const float a = v (amp ? ids::aA : ids::fA), d = v (amp ? ids::aD : ids::fD), s = amp ? std::sqrt (jmax (0.0f, v (ids::aS))) : v (ids::fS), r = v (amp ? ids::aR : ids::fR);
        auto tm = [] (float k) { return 0.002f * std::pow (3000.0f, k); };
        const float A = tm (a), D = tm (d), R = tm (r), hold = 0.5f, total = A + D + hold + R;
        auto X = [&] (float t) { return 5.0f + t / total * (w - 10.0f); }; auto Y = [&] (float val) { return h - 8.0f - val * (h - 20.0f); };
        Path p; p.startNewSubPath (X (0), Y (0)); const int steps = 40;
        for (int i = 1; i <= steps; ++i) { float val = 1.25f - 1.25f * std::exp (-3.2f * i / steps); val = jmin (val / (1.25f * (1.0f - std::exp (-3.2f))), 1.0f); p.lineTo (X (A * i / steps), Y (val)); }
        for (int i = 1; i <= steps; ++i) p.lineTo (X (A + D * i / steps), Y (s + (1.0f - s) * std::exp (-4.0f * i / steps)));
        p.lineTo (X (A + D + hold), Y (s));
        for (int i = 1; i <= steps; ++i) p.lineTo (X (A + D + hold + R * i / steps), Y (s * std::exp (-4.0f * i / steps)));
        Path fill (p); fill.lineTo (X (total), Y (0)); fill.lineTo (X (0), Y (0)); fill.closeSubPath();
        g.setColour (c.withAlpha (0.13f)); g.fillPath (fill); g.setColour (c); g.strokePath (p, PathStrokeType (2.0f));
        g.setColour (Colour (0xff4a4d58)); const float xh = X (A + D + hold);
        for (float y = 0.0f; y < h; y += 6.0f) g.drawVerticalLine ((int) xh, y, y + 3.0f);
        g.setColour (col::inkDim); g.setFont (monoFont (9.0f));
        g.drawText (String (amp ? "Amp" : "Filter") + "  total " + String (total, 2) + " s, key held 0.5 s", 6, 2, (int) w - 8, 11, Justification::left, false);
    }
};

//==============================================================================
class Meter : public Component
{
public:
    Meter (PurrOneProcessor& p) : proc (p) { setBounds (rc (layout::misc.meter)); setInterceptsMouseClicks (false, false); }
    void tick()
    {
        auto up = [] (float& d, float v) { d = v > d ? v : d * 0.92f; };
        up (dispL, proc.meterL.load()); up (dispR, proc.meterR.load());
        holdL = jmax (dispL, holdL * 0.985f); holdR = jmax (dispR, holdR * 0.985f);
        if (proc.clipped.exchange (false)) clipFrames = 45; else if (clipFrames > 0) --clipFrames;
        repaint();
    }
    void paint (Graphics& g) override
    {
        const auto m = layout::misc; const int ox = m.meter.x, oy = m.meter.y;
        g.setColour (col::panel2); g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        g.setColour (col::line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
        g.setColour (col::ink); g.setFont (labelFont (14.0f, true, 0.2f)); g.drawText ("OUT", 12, 8, 60, 18, Justification::left, false);
        auto bar = [&] (const layout::R& rr, float lin, float hold)
        {
            const auto r = rc (rr).translated (-ox, -oy).toFloat();
            g.setColour (col::black); g.fillRect (r); g.setColour (col::line); g.drawRect (r, 1.0f);
            const auto in = r.reduced (1.0f);
            auto frac = [] (float l) { const float db = l > 0.00001f ? 20.0f * std::log10 (l) : -90.0f; return jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };
            const float f = frac (lin); const float top = in.getBottom() - f * in.getHeight();
            struct Band { float from, to; Colour c; };
            for (auto b : { Band { 0.0f, 0.70f, col::green }, Band { 0.70f, 0.87f, col::yellow }, Band { 0.87f, 0.95f, col::orange }, Band { 0.95f, 1.0f, col::pink } })
            {
                const float y0 = in.getBottom() - b.to * in.getHeight(), y1 = in.getBottom() - b.from * in.getHeight();
                const float a = jmax (y0, top); if (a < y1) { g.setColour (b.c); g.fillRect (in.getX(), a, in.getWidth(), y1 - a); }
            }
            g.setColour (col::ink); g.fillRect (in.getX(), in.getBottom() - frac (hold) * in.getHeight() - 1.0f, in.getWidth(), 2.0f);
        };
        bar (m.barL, dispL, holdL); bar (m.barR, dispR, holdR);
        g.setFont (monoFont (10.0f)); g.setColour (col::inkDim);
        const auto sc = rc (m.scale).translated (-ox, -oy);
        for (int db : { 0, -6, -12, -18, -24, -36, -48, -60 })
        {
            const float y = (float) sc.getBottom() - (float) (db + 60) / 60.0f * (float) sc.getHeight();
            g.drawHorizontalLine ((int) y, (float) sc.getX() - 6.0f, (float) sc.getX() - 2.0f);
            g.drawText (String (db), sc.getX(), (int) y - 6, 34, 12, Justification::left, false);
        }
        g.setFont (monoFont (11.0f)); g.drawText ("L R dBFS", 12, getHeight() - 24, 70, 14, Justification::left, false);
        const Rectangle<float> led ((float) getWidth() - 70.0f, (float) getHeight() - 22.0f, 10.0f, 10.0f);
        if (clipFrames > 0) { g.setColour (col::pink.withAlpha (0.4f)); g.fillEllipse (led.expanded (3.0f)); g.setColour (col::pink); } else g.setColour (Colour (0xff2a2c33));
        g.fillEllipse (led); g.setColour (col::inkDim); g.drawText ("clip", (int) led.getRight() + 5, getHeight() - 24, 40, 14, Justification::left, false);
    }
private:
    PurrOneProcessor& proc; float dispL = 0, dispR = 0, holdL = 0, holdR = 0; int clipFrames = 0;
};

//==============================================================================
// Keyboard with the mockup's colours; Z and X shift the octave of the computer keys.
class Keys : public MidiKeyboardComponent
{
public:
    Keys (MidiKeyboardState& s) : MidiKeyboardComponent (s, horizontalKeyboard)
    {
        setAvailableRange (36, 84); setScrollButtonsVisible (false); setBlackNoteLengthProportion (0.56f); setBlackNoteWidthProportion (0.6f);
        setKeyPressBaseOctave (3); setMidiChannel (1); setMidiChannelsToDisplay (0xffff);
        setColour (whiteNoteColourId, Colour (0xffdcdee6)); setColour (blackNoteColourId, Colour (0xff111216));
        setColour (keySeparatorLineColourId, Colour (0x66000000)); setColour (shadowColourId, Colours::transparentBlack);
        setColour (keyDownOverlayColourId, col::green.withAlpha (0.75f)); setColour (mouseOverKeyOverlayColourId, Colours::white.withAlpha (0.12f));
        setColour (upDownButtonBackgroundColourId, col::panel2);
        setWantsKeyboardFocus (true);
    }
    void resized() override { const auto n = getRangeEnd() - getRangeStart(); (void) n; setKeyWidth ((float) getWidth() / 29.0f); MidiKeyboardComponent::resized(); }
    bool keyPressed (const KeyPress& k) override
    {
        const auto c = k.getTextCharacter();
        if (c == 'z' || c == 'Z') { octave = jmax (0, octave - 1); setKeyPressBaseOctave (octave); return true; }
        if (c == 'x' || c == 'X') { octave = jmin (7, octave + 1); setKeyPressBaseOctave (octave); return true; }
        return MidiKeyboardComponent::keyPressed (k);
    }
    void mouseEnter (const MouseEvent& e) override { grabKeyboardFocus(); MidiKeyboardComponent::mouseEnter (e); }
private:
    int octave = 3;
};

} // namespace purrone::ui
