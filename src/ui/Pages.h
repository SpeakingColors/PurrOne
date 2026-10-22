// The Seq / Arp, FX and Settings pages. Positions come from Layout2.h (generated from the mockup); behaviour lives here.
#pragma once
#include <complex>
#include <juce_dsp/juce_dsp.h>
#include "Widgets.h"
#include "Layout2.h"
#include "../PluginProcessor.h"
#include "../Presets.h"
#include "../FxPresets.h"
#include "purrone/Engine.h"

namespace purrone::ui
{
namespace L2 = purrone::layout2;

inline void paintSection (Graphics& g, const layout::Sec& s, const String& title, const String& sub)
{
    const auto r = rc (s.r).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (Colour (0xff3a3d48)); g.fillRect (r.getX() + 1.0f, r.getY(), r.getWidth() - 2.0f, 3.0f);
    g.setColour (col::ink); g.setFont (labelFont (16.0f, true, 0.2f)); g.drawText (title, (int) r.getX() + 10, (int) r.getY() + 8, 260, 18, Justification::centredLeft, false);
    g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.1f)); g.drawText (sub, (int) r.getX() + 200, (int) r.getY() + 8, (int) r.getWidth() - 210, 18, Justification::centredRight, true);
}

inline const L2::Item& itemOf (const L2::ViewData& v, const char* kind, const char* text = "")
{
    for (int i = 0; i < v.nItems; ++i) if (String (v.items[i].kind) == kind && (String (text).isEmpty() || String (v.items[i].text) == text)) return v.items[i];
    jassertfalse; return v.items[0];
}
inline void setReal (RangedAudioParameter* p, float real) { if (p != nullptr) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
inline float realOf (RangedAudioParameter* p) { return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f; }
inline String noteName (int midi) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return String (n[((midi % 12) + 12) % 12]) + String (midi / 12 - 1); }

//==============================================================================
class TextBtn : public Component, public SettableTooltipClient
{
public:
    TextBtn (const String& t, const String& tip = {}) : text (t) { setMouseCursor (MouseCursor::PointingHandCursor); setTooltip (tip); }
    void paint (Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (on ? Colour (0xff1f3a33) : (over ? Colour (0xff23252c) : col::panel2)); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (on ? Colour (0xff2c6a59) : (over ? Colour (0xff4a4d58) : col::line)); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (on ? col::green : col::ink); g.setFont (labelFont (14.0f, true, 0.08f)); g.drawText (text.toUpperCase(), getLocalBounds().reduced (4, 0), Justification::centred, true);
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
    String text; std::function<void()> onClick; bool over = false, on = false;
};

// segmented control for editor-only state (band selector, analyzer tabs, window size...)
class LocalSeg : public Component, public SettableTooltipClient
{
public:
    LocalSeg (const layout::Seg& d, const layout::Btn* base, Bubbles& b, std::function<int()> g, std::function<void(int)> s) : def (d), btns (base), bubbles (b), get (std::move (g)), set (std::move (s))
    {
        setBounds (rc (def.r)); setMouseCursor (MouseCursor::PointingHandCursor);
        if (String (def.tip).isNotEmpty()) setTooltip (def.tip);
    }
    String getTooltip() override { return bubbles.on ? SettableTooltipClient::getTooltip() : String(); }
    void tick() { const int c = get(); if (c != last) { last = c; repaint(); } }
    Rectangle<int> btn (int i) const { return rc (btns[def.first + i].r).translated (-def.r.x, -def.r.y); }
    void paint (Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat(); const int cur = get();
        { Graphics::ScopedSaveState s (g); Path clip; clip.addRoundedRectangle (all, 3.0f); g.reduceClipRegion (clip); g.fillAll (col::field);
          for (int i = 0; i < def.n; ++i)
          {
              const bool sel = i == cur; const auto b = btn (i);
              if (sel) { g.setColour (col::selected); g.fillRect (b); }
              g.setColour (sel ? col::ink : col::inkDim); g.setFont (labelFont (13.0f, true, 0.08f)); g.drawText (btns[def.first + i].text, b, Justification::centred, false);
          } }
        g.setColour (col::line); g.drawRoundedRectangle (all.reduced (0.5f), 3.0f, 1.0f);
    }
    void mouseDown (const MouseEvent& e) override { for (int i = 0; i < def.n; ++i) if (btn (i).contains (e.getPosition())) { set (i); last = -99; repaint(); return; } }
private:
    const layout::Seg& def; const layout::Btn* btns; Bubbles& bubbles; std::function<int()> get; std::function<void(int)> set; int last = -99;
};

// - value + control
class Stepper : public Component, public SettableTooltipClient
{
public:
    Stepper (Rectangle<int> r, std::function<String()> txt, std::function<void(int)> step, const String& tip = {}) : text (std::move (txt)), stepFn (std::move (step))
    { setBounds (r); setMouseCursor (MouseCursor::PointingHandCursor); setTooltip (tip); }
    void tick() { const auto t = text(); if (t != last) { last = t; repaint(); } }
    void paint (Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        { Graphics::ScopedSaveState s (g); Path clip; clip.addRoundedRectangle (all, 3.0f); g.reduceClipRegion (clip);
          g.fillAll (col::field); g.setColour (col::black); g.fillRect (getLocalBounds().withTrimmedLeft (30).withTrimmedRight (30));
          g.setColour (col::ink); g.setFont (monoFont (15.0f));
          g.drawText ("-", 0, 0, 30, getHeight(), Justification::centred, false); g.drawText ("+", getWidth() - 30, 0, 30, getHeight(), Justification::centred, false);
          g.drawText (text(), 30, 0, getWidth() - 60, getHeight(), Justification::centred, false); }
        g.setColour (col::line); g.drawRoundedRectangle (all.reduced (0.5f), 3.0f, 1.0f);
    }
    void mouseDown (const MouseEvent& e) override { const int d = e.mods.isShiftDown() ? 4 : 1; if (e.x < 30) stepFn (-d); else if (e.x > getWidth() - 30) stepFn (d); repaint(); }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { if (w.deltaY != 0.0f) { stepFn (w.deltaY > 0 ? 1 : -1); repaint(); } }
private:
    std::function<String()> text; std::function<void(int)> stepFn; String last;
};

// small horizontal slider with a value readout
class MiniSlider : public Component, public SettableTooltipClient
{
public:
    MiniSlider (RangedAudioParameter& p, Rectangle<int> r, const String& tip = {}) : param (p), attach (p, [this] (float) { repaint(); }) { setBounds (r); attach.sendInitialUpdate(); setTooltip (tip); setMouseCursor (MouseCursor::UpDownLeftRightResizeCursor); }
    void paint (Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat(); const float n = param.getValue();
        g.setColour (col::field); g.fillRoundedRectangle (r, 3.0f); g.setColour (col::green.withAlpha (0.35f)); g.fillRoundedRectangle (r.withWidth (jmax (4.0f, r.getWidth() * n)), 3.0f);
        g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (col::ink); g.setFont (labelFont (13.0f, true, 0.08f)); g.drawText ("AMOUNT " + String (roundToInt (n * 100.0f)) + "%", getLocalBounds(), Justification::centred, false);
    }
    void mouseDown (const MouseEvent& e) override { attach.beginGesture(); set (e); }
    void mouseDrag (const MouseEvent& e) override { set (e); }
    void mouseUp (const MouseEvent&) override { attach.endGesture(); }
private:
    void set (const MouseEvent& e) { attach.setValueAsPartOfGesture (param.convertFrom0to1 (jlimit (0.0f, 1.0f, (float) e.x / (float) getWidth()))); }
    RangedAudioParameter& param; ParameterAttachment attach;
};

//==============================================================================
// Shared base: creates the knobs, toggles and segments listed for a view and paints the sections and labels.
class PageBase : public Component
{
public:
    PageBase (PurrOneProcessor& p, Bubbles& b, const L2::ViewData& v) : proc (p), bubbles (b), vd (v) { setInterceptsMouseClicks (false, true); setBounds (0, 0, layout::W, layout::H); }

    template <typename Skip> void build (Skip skip)
    {
        auto& a = proc.apvts;
        for (int i = 0; i < vd.nKnobs; ++i) if (! skip (vd.knobs[i].id)) if (auto* p = a.getParameter (vd.knobs[i].id)) { auto* k = new Knob (*p, vd.knobs[i], bubbles); owned.add (k); addAndMakeVisible (k); }
        for (int i = 0; i < vd.nTogs; ++i)  if (! skip (vd.togs[i].id))  if (auto* p = a.getParameter (vd.togs[i].id))  { auto* t = new LedToggle (*p, vd.togs[i], bubbles); owned.add (t); addAndMakeVisible (t); }
        for (int i = 0; i < vd.nSegs; ++i)  if (! skip (vd.segs[i].id))  if (auto* p = a.getParameter (vd.segs[i].id))  { auto* s = new Segments (*p, vd.segs[i], bubbles, L2::segBtns2); owned.add (s); addAndMakeVisible (s); }
    }
    const layout::Seg* segDef (const char* id) const { for (int i = 0; i < vd.nSegs; ++i) if (String (vd.segs[i].id) == id) return &vd.segs[i]; return nullptr; }
    LocalSeg* addLocal (const char* id, std::function<int()> g, std::function<void(int)> s)
    {
        auto* d = segDef (id); if (d == nullptr) return nullptr;
        auto* l = new LocalSeg (*d, L2::segBtns2, bubbles, std::move (g), std::move (s)); owned.add (l); locals.add (l); addAndMakeVisible (l); return l;
    }
    template <typename T> T* add (T* c) { owned.add (c); addAndMakeVisible (c); return c; }

    virtual String secTitle (int i) const { return vd.secs[i].title; }
    virtual String secSub (int i) const { return vd.secs[i].sub; }
    virtual bool drawsSection (int) const { return true; }
    virtual void tick() { for (auto* l : locals) l->tick(); for (auto* c : owned) if (auto* st = dynamic_cast<Stepper*> (c)) st->tick(); }

    void paint (Graphics& g) override
    {
        for (int i = 0; i < vd.nSecs; ++i) if (drawsSection (i)) paintSection (g, vd.secs[i], secTitle (i), secSub (i));
        for (int i = 0; i < vd.nItems; ++i)
        {
            const auto& it = vd.items[i]; const String k (it.kind), t (it.text);
            if (k == "lab" || k == "rowlabel") { g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.12f)); g.drawText (t.toUpperCase(), rc (it.r), Justification::centredLeft, false); }
            else if (k == "note" && ! t.startsWith ("Reads the final") && ! t.startsWith ("Signal order") && showNote (t)) { g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.04f)); g.drawFittedText (noteText (t), rc (it.r), Justification::topLeft, 3, 1.0f); }
        }
    }
    virtual bool showNote (const String&) const { return true; }
    virtual String noteText (const String& t) const { return t; }

    void askText (const String& title, const String& msg, const String& def, const String& ok, bool readOnly, std::function<void (String)> done)
    {
        dialog = std::make_unique<AlertWindow> (title, msg, MessageBoxIconType::NoIcon);
        dialog->addTextEditor ("t", def);
        if (auto* te = dialog->getTextEditor ("t")) { te->setReadOnly (readOnly); if (readOnly) te->selectAll(); }
        dialog->addButton (ok, 1, KeyPress (KeyPress::returnKey)); dialog->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        dialog->enterModalState (true, ModalCallbackFunction::create ([this, done] (int r)
        { const auto txt = dialog != nullptr ? dialog->getTextEditorContents ("t") : String(); dialog.reset(); if (r == 1 && done) done (txt); }), false);
    }
    void say (const String& title, const String& msg) { AlertWindow::showAsync (MessageBoxOptions().withIconType (MessageBoxIconType::InfoIcon).withTitle (title).withMessage (msg).withButton ("OK"), nullptr); }

    PurrOneProcessor& proc; Bubbles& bubbles; const L2::ViewData& vd;
    OwnedArray<Component> owned; Array<LocalSeg*> locals; std::unique_ptr<AlertWindow> dialog;
};

//==============================================================================
// Seq / Arp
static const int kStepChoices[] = { 1, 2, 4, 8, 12, 16, 24, 32, 48, 64 };

struct FacSeq { const char* name; String spec; float gate; int rate; };
inline const std::vector<FacSeq>& factorySequences()
{
    static const std::vector<FacSeq> list = []
    {
        std::vector<FacSeq> v;
        auto nm = [] (int semi, bool a, bool s) { return noteName (36 + semi) + String (a ? "a" : "") + String (s ? "s" : ""); };
        // note names in the notation are C2..C5 with C2 the lowest: noteName (36) is "C2" here, so it matches
        auto make = [&] (int n, auto semi, auto gate, auto acc, auto slide)
        {
            String s; for (int i = 0; i < n; ++i) { if (i) s << " "; s << (gate (i) ? nm (semi (i), acc (i), slide (i)) : String ("-")); } return s;
        };
        const int roll[8] = { 0, 0, 12, 0, 7, 0, 12, 0 };
        const int acid[16] = { 0, 0, 12, 0, 3, 0, 12, 5, 0, 0, 7, 0, 12, 10, 0, 3 };
        v.push_back ({ "Acid 16", make (16, [&] (int i) { return acid[i]; }, [] (int i) { return i != 1 && i != 9; }, [] (int i) { return i == 2 || i == 10 || i == 12; }, [] (int i) { return i == 7 || i == 13; }), 0.6f, 2 });
        v.push_back ({ "Offbeat Bass", make (16, [] (int) { return 0; }, [] (int i) { return i % 4 == 2; }, [] (int) { return false; }, [] (int) { return false; }), 0.5f, 2 });
        v.push_back ({ "Rolling 16ths", make (16, [&] (int i) { return roll[i % 8]; }, [] (int) { return true; }, [] (int i) { return i % 8 == 0; }, [] (int) { return false; }), 0.45f, 2 });
        v.push_back ({ "Gallop 12", make (12, [] (int) { return 0; }, [] (int i) { return i % 3 != 1; }, [] (int i) { return i % 6 == 0; }, [] (int) { return false; }), 0.4f, 2 });
        v.push_back ({ "Pulse 1 step", make (1, [] (int) { return 0; }, [] (int) { return true; }, [] (int) { return true; }, [] (int) { return false; }), 0.7f, 0 });
        v.push_back ({ "Rolling 32", make (32, [&] (int i) { return roll[i % 8] + (i >= 16 ? 3 : 0); }, [] (int) { return true; }, [] (int i) { return i % 8 == 0; }, [] (int i) { return i % 16 == 11; }), 0.4f, 2 });
        return v;
    }();
    return list;
}

class SeqLanes : public Component
{
public:
    SeqLanes (PurrOneProcessor& p, int& pageRef) : proc (p), page (pageRef) { setBounds (rc (itemOf (L2::views[L2::vSeq], "lanes").r)); }
    int pat() const { return (int) std::lround (proc.apvts.getRawParameterValue (ids::seqPat)->load()); }
    int steps() const { return (int) std::lround (proc.apvts.getRawParameterValue (ids::seqSteps)->load()); }
    static constexpr int kLabW = 82, kRows = 7, kRowY[7] = { 0, 21, 42, 63, 84, 105, 126 }, kRowH[7] = { 19, 20, 20, 20, 20, 20, 20 };
    float pitch() const { return (float) (getWidth() - kLabW + 4) / 16.0f; }
    Rectangle<float> cell (int cx, int row) const { return { (float) kLabW + cx * pitch(), (float) kRowY[row], pitch() - 4.0f, (float) kRowH[row] }; }
    void tick() { const int ph = proc.playStep.load(); const uint64 sig = (uint64) (ph + 2) * 1000003ull + (uint64) pat() * 977 + (uint64) steps() * 31 + (uint64) page; uint64 h = sig;
                  for (int c = 0; c < 16; ++c) h = h * 1099511628211ull + proc.seq.get (pat(), page * 16 + c);
                  if (h != lastHash) { lastHash = h; repaint(); } }
    void paint (Graphics& g) override
    {
        const int n = steps(), ph = proc.playStep.load(), pt = pat();
        for (int c = 0; c < 16; ++c)
        {
            const int i = page * 16 + c; const bool inside = i < n; const uint16_t v = proc.seq.get (pt, i);
            const bool playing = ph == i;
            auto fill = [&] (int row, Colour onC, bool lit, const String& txt)
            {
                const auto r = cell (c, row);
                g.setColour (lit && inside ? onC : col::field); g.fillRoundedRectangle (r, 3.0f);
                g.setColour (playing ? col::blue : col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, playing ? 2.0f : 1.0f);
                if (txt.isNotEmpty()) { g.setColour (inside ? (lit ? col::black : col::ink) : col::inkDim.withAlpha (0.4f)); g.setFont (labelFont (14.0f, true, 0.06f)); g.drawText (txt, r.toNearestInt(), Justification::centred, false); }
            };
            { const auto r = cell (c, 0); g.setColour (playing ? col::blue : (inside ? col::ink : col::inkDim.withAlpha (0.35f))); g.setFont (monoFont (13.0f)); g.drawText (i < 64 ? String (i + 1) : String(), r.toNearestInt(), Justification::centred, false); }
            const int nt = SeqData::note (v);
            { const auto r = cell (c, 1); const bool gate = SeqData::gate (v);
              g.setColour (col::field); g.fillRoundedRectangle (r, 3.0f);
              if (inside) { g.setColour (Colour (0xff1f3a33).withAlpha (gate ? 1.0f : 0.5f)); g.fillRoundedRectangle (r.withWidth (jmax (3.0f, r.getWidth() * (float) (nt + 1) / 37.0f)), 3.0f); }
              g.setColour (playing ? col::blue : col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, playing ? 2.0f : 1.0f);
              g.setColour (inside ? (gate ? col::ink : col::inkDim) : col::inkDim.withAlpha (0.4f)); g.setFont (monoFont (14.0f)); g.drawText (noteName (36 + nt), r.toNearestInt(), Justification::centred, false); }
            fill (2, col::green, SeqData::gate (v), SeqData::gate (v) ? "ON" : "");
            fill (3, col::orange, SeqData::accent (v), SeqData::accent (v) ? "ACC" : "");
            fill (4, col::blue, SeqData::slide (v), SeqData::slide (v) ? "SLD" : "");
            { const int ch = SeqData::chance (v); static const char* const chTxt[4] = { "", "75%", "50%", "25%" }; fill (5, col::yellow, ch != 0, chTxt[ch]); }
            { const int rt = SeqData::ratchet (v); fill (6, col::pink, rt > 1, rt > 1 ? "x" + String (rt) : String()); }
        }
    }
    bool hit (Point<int> p, int& col_, int& row) const
    {
        for (int c = 0; c < 16; ++c) for (int r = 1; r < kRows; ++r) if (cell (c, r).expanded (2.0f, 1.0f).contains (p.toFloat())) { col_ = c; row = r; return true; }
        return false;
    }
    void mouseDown (const MouseEvent& e) override
    {
        dragCol = -1; if (! hit (e.getPosition(), dragCol, dragRow)) return;
        const int i = page * 16 + dragCol; if (i >= steps()) { dragCol = -1; return; }
        startV = proc.seq.get (pat(), i); moved = false;
        if (dragRow >= 2) { toggle (i, dragRow, e.mods.isShiftDown() || e.mods.isRightButtonDown()); dragCol = -1; }
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragCol < 0 || dragRow != 1) return;
        if (std::abs (e.getDistanceFromDragStartY()) < 4) return;
        moved = true; const int i = page * 16 + dragCol;
        const int nt = jlimit (0, 36, SeqData::note (startV) - e.getDistanceFromDragStartY() / 8);
        proc.seq.set (pat(), i, (uint16_t) ((startV & ~63) | nt)); repaint();
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragCol >= 0 && dragRow == 1 && ! moved)
        {
            const int i = page * 16 + dragCol; const int nt = SeqData::note (startV);
            const int next = (e.mods.isShiftDown() || e.mods.isRightButtonDown()) ? jmax (0, nt - 1) : jmin (36, nt + 1);
            proc.seq.set (pat(), i, (uint16_t) ((startV & ~63) | next)); repaint();
        }
        dragCol = -1;
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        int c, r; if (! hit (e.getPosition(), c, r) || r != 1 || w.deltaY == 0.0f) return;
        const int i = page * 16 + c; if (i >= steps()) return; const auto v = proc.seq.get (pat(), i);
        proc.seq.set (pat(), i, (uint16_t) ((v & ~63) | jlimit (0, 36, SeqData::note (v) + (w.deltaY > 0 ? 1 : -1)))); repaint();
    }
private:
    void toggle (int i, int row, bool back)
    {
        auto v = proc.seq.get (pat(), i);
        if (row >= 5)                                              // chance and ratchet cycle through their four values; shift or right click goes back
        {
            const int shift = row == 5 ? 9 : 11; const int cur = (v >> shift) & 3, next = (cur + (back ? 3 : 1)) & 3;
            proc.seq.set (pat(), i, (uint16_t) ((v & ~(3 << shift)) | (next << shift))); repaint(); return;
        }
        const uint16_t bit = row == 2 ? 64 : row == 3 ? 128 : 256; proc.seq.set (pat(), i, (uint16_t) (v ^ bit)); repaint();
    }
    PurrOneProcessor& proc; int& page; int dragCol = -1, dragRow = 0; uint16_t startV = 0; bool moved = false; uint64 lastHash = 1;
};

class SeqList : public Component
{
public:
    SeqList (PurrOneProcessor& p, Rectangle<int> r) : proc (p) { setBounds (r); rebuild(); }
    void rebuild()
    {
        rows.clear(); for (auto& f : factorySequences()) rows.add ({ f.name, true });
        for (auto& u : proc.listSequences()) rows.add ({ u, false });
        repaint();
    }
    int selected = -1; std::function<void(int)> onPick;
    String selectedName() const { return selected >= 0 && selected < rows.size() ? rows[selected].name : String(); }
    bool selectedIsFactory() const { return selected >= 0 && selected < rows.size() && rows[selected].factory; }
    void paint (Graphics& g) override
    {
        g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        for (int i = 0; i < rows.size(); ++i)
        {
            const auto r = Rectangle<int> (1, 1 + (i - scroll) * 23, getWidth() - 2, 22); if (r.getBottom() > getHeight() || r.getY() < 0) continue;
            if (i == selected) { g.setColour (Colour (0xff1f3a33)); g.fillRect (r); }
            g.setColour (i == selected ? col::green : col::ink); g.setFont (monoFont (13.0f)); g.drawText (rows[i].name, r.reduced (8, 0), Justification::centredLeft, true);
            g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText (rows[i].factory ? "factory" : "yours", r.reduced (8, 0), Justification::centredRight, false);
        }
    }
    void mouseDown (const MouseEvent& e) override { const int i = scroll + (e.y - 1) / 23; if (i >= 0 && i < rows.size()) { selected = i; repaint(); if (onPick) onPick (i); } }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { scroll = jlimit (0, jmax (0, rows.size() - 4), scroll + (w.deltaY > 0 ? -1 : 1)); repaint(); }
private:
    struct Row { String name; bool factory; };
    PurrOneProcessor& proc; Array<Row> rows; int scroll = 0;
};

class SeqPage : public PageBase
{
public:
    SeqPage (PurrOneProcessor& p, Bubbles& b) : PageBase (p, b, L2::views[L2::vSeq])
    {
        build ([] (const char* id) { return String (id) == "seqPage" || String (id) == "seqStepsSel"; });
        addLocal ("seqPage", [this] { return page; }, [this] (int v) { page = v; });
        addLocal ("seqStepsSel", [this] { const int n = steps(); for (int i = 0; i < 10; ++i) if (kStepChoices[i] == n) return i; return -1; },
                  [this] (int v) { proc.setParam (ids::seqSteps, (float) kStepChoices[jlimit (0, 9, v)]); });
        lanes = add (new SeqLanes (proc, page));
        add (new Stepper (rc (itemOf (vd, "stepper").r), [this] { return String (steps()); }, [this] (int d) { proc.setParam (ids::seqSteps, (float) jlimit (1, 64, steps() + d)); }, "Number of steps, 1 to 64. Shift+click jumps by 4."));
        play = add (new TextBtn ("Play", "Starts and stops the sequence by hand. In a DAW the transport, a key or a start note can do it too (see Start below).")); play->setBounds (rc (itemOf (vd, "btn", "Play").r));
        play->onClick = [this] { const bool on = ! proc.playButton.load(); if (on && mode() == 0) proc.setParam (ids::seqMode, 1.0f); proc.playButton.store (on); };
        auto btn = [this] (const char* label, const char* tip, std::function<void()> fn) { auto* t = add (new TextBtn (label, tip)); t->setBounds (rc (itemOf (vd, "btn", label).r)); t->onClick = std::move (fn); return t; };
        btn ("Random", "Fills this pattern with a random line in an acid style.", [this] { randomise(); });
        btn ("Clear", "Empties this pattern.", [this] { for (int i = 0; i < 64; ++i) proc.seq.set (pat(), i, SeqData::pack (0, false, false, false)); });
        btn ("Copy", "Copies this pattern.", [this] { for (int i = 0; i < 64; ++i) clip[(size_t) i] = proc.seq.get (pat(), i); haveClip = true; });
        btn ("Paste", "Pastes the copied pattern here.", [this] { if (haveClip) for (int i = 0; i < 64; ++i) proc.seq.set (pat(), i, clip[(size_t) i]); });
        list = add (new SeqList (proc, rc (itemOf (vd, "list").r)));
        list->onPick = [this] (int i) { loadRow (i); };
        btn ("Save as...", "Stores the sequence and its settings in your own sequence folder. It works with any sound.", [this] { saveAs(); });
        btn ("Delete", "Deletes your selected sequence. Factory ones stay.", [this] { if (! list->selectedIsFactory() && list->selectedName().isNotEmpty()) { proc.deleteSequence (list->selectedName()); list->selected = -1; list->rebuild(); } });
        btn ("Share", "Gives you a text code for the current sequence. Send it to a friend; Import reads it back.", [this] { share(); });
        btn ("Import", "Paste a sequence code that someone sent you.", [this] { importCode(); });
        // start options (not in the mockup grid: they sit in the free row under the lanes)
        static const layout::Tog t[] = {
            { "seqOnHost", "START WITH HOST", { 61, 449, 168, 26 }, 0xff02ebbau, "Starts when the DAW plays and follows its tempo. Needs Sync set to Host." },
            { "seqOnKey", "START ON KEY", { 235, 449, 140, 26 }, 0xff02ebbau, "Pressing a key starts the sequence. It plays the pattern transposed to that key and stops when you let go." },
            { "seqOnNote", "START NOTE", { 381, 449, 128, 26 }, 0xff02ebbau, "A chosen note toggles the sequence on and off. That note is not played as a sound." },
            { "seqKeyTrans", "KEYS TRANSPOSE", { 660, 449, 158, 26 }, 0xffffd100u, "The lowest key you hold moves the whole pattern. C2 plays it as written." },
            { "squelch", "303 SQUELCH", { 900, 449, 140, 26 }, 0xffff1383u, "Accents and slides push the resonance up for a moment: the classic acid squelch. Off by default." } };
        for (auto& d : t) if (auto* prm = proc.apvts.getParameter (d.id)) add (new LedToggle (*prm, d, bubbles));
        add (new Stepper ({ 515, 449, 138, 26 }, [this] { return noteName (jlimit (0, 127, (int) std::lround (proc.apvts.getRawParameterValue (ids::seqTrigNote)->load()))); },
                          [this] (int d) { proc.setParam (ids::seqTrigNote, (float) jlimit (0, 127, (int) std::lround (proc.apvts.getRawParameterValue (ids::seqTrigNote)->load()) + d)); }, "The note that starts and stops the sequence when START NOTE is on."));
        if (auto* sq = proc.apvts.getParameter (ids::squelchAmt)) add (new MiniSlider (*sq, { 1046, 449, 210, 26 }, "How hard accents and slides push the resonance."));
        auto oct = [this] (const char* label, int d) { auto* t = add (new TextBtn (label, d > 0 ? "Transposes the whole sequence up one octave. The Transpose knob moves it in semitones (3 octaves each way)." : "Transposes the whole sequence down one octave. The Transpose knob moves it in semitones (3 octaves each way).")); t->setBounds (d > 0 ? Rectangle<int> (1356, 449, 82, 26) : Rectangle<int> (1266, 449, 82, 26));
            t->onClick = [this, d] { proc.setParam (ids::seqTrans, (float) jlimit (-36, 36, (int) std::lround (proc.apvts.getRawParameterValue (ids::seqTrans)->load()) + d * 12)); }; };
        oct ("OCT -", -1); oct ("OCT +", 1);
        mode_ = proc.apvts.getRawParameterValue (ids::seqMode);
    }
    int lastAcc = 0, accFlash = 0;
    int mode() const { return (int) std::lround (mode_->load()); }
    int steps() const { return (int) std::lround (proc.apvts.getRawParameterValue (ids::seqSteps)->load()); }
    int pat() const { return (int) std::lround (proc.apvts.getRawParameterValue (ids::seqPat)->load()); }
    void tick() override
    {
        PageBase::tick(); lanes->tick();
        play->text = proc.playButton.load() ? "Stop" : "Play"; const bool on = proc.playButton.load(); if (play->on != on) { play->on = on; } play->repaint();
        const bool led = proc.apvts.getRawParameterValue (ids::midiOut)->load() > 0.5f && proc.seqRunning.load() && proc.playStep.load() >= 0;
        if (led != ledOn) { ledOn = led; repaint(); }
        const int ph = proc.playStep.load(); if (ph >= 0 && followPlay && proc.seqRunning.load() && ph / 16 != page && ph != lastPh) page = jmin (3, ph / 16);
        lastPh = ph;
        const int ac = proc.accentCount.load (std::memory_order_relaxed);
        if (ac != lastAcc) { lastAcc = ac; accFlash = 4; repaint(); } else if (accFlash > 0 && --accFlash == 0) repaint();   // about 130 ms at 30 Hz
    }
    void paint (Graphics& g) override
    {
        PageBase::paint (g);
        { const auto k = Rectangle<float> (1159.0f, 104.0f, 62.0f, 74.0f); const Point<float> c (k.getCentreX() + 24.0f, k.getY() + 6.0f);   // accent light, top right of the Accent knob
          if (accFlash > 0) { g.setColour (col::orange.withAlpha (0.4f)); g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f); g.setColour (col::orange); } else g.setColour (Colour (0xff2a2c33));
          g.fillEllipse (c.x - 4.0f, c.y - 4.0f, 8.0f, 8.0f); }
        const auto r = rc (itemOf (vd, "led").r).toFloat();
        if (ledOn) { g.setColour (col::pink.withAlpha (0.4f)); g.fillEllipse (r.expanded (3.0f)); g.setColour (col::pink); } else g.setColour (Colour (0xff2a2c33));
        g.fillEllipse (r);
        g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.12f)); g.drawText ("START", Rectangle<int> (61, 424, 60, 22), Justification::centredLeft, false);
    }
    bool showNote (const String&) const override { return false; }
private:
    void randomise()
    {
        Random rnd; const int sc[] = { 0, 0, 0, 3, 5, 7, 10, 12 };
        for (int i = 0; i < 64; ++i)
        {
            const bool gate = rnd.nextFloat() < 0.8f;
            proc.seq.set (pat(), i, SeqData::pack (sc[rnd.nextInt (8)], gate, gate && rnd.nextFloat() < 0.25f, gate && rnd.nextFloat() < 0.2f));
        }
    }
    void loadRow (int i)
    {
        if (list->selectedIsFactory())
        {
            const auto& f = factorySequences()[(size_t) i];
            proc.setParam (ids::seqPat, 0.0f); proc.applySeqSpec (f.spec); proc.setParam (ids::seqGate, f.gate); proc.setParam (ids::seqRate, (float) f.rate);
        }
        else proc.loadSequence (list->selectedName());
        page = 0;
    }
    void saveAs()
    {
        askText ("Save sequence", "Name for this sequence", list->selectedIsFactory() || list->selectedName().isEmpty() ? "My sequence" : list->selectedName(), "Save", false, [this] (String nm)
        { if (nm.trim().isNotEmpty() && proc.saveSequence (nm)) { list->rebuild(); } });
    }
    void share()
    {
        const auto code = proc.sequenceCode (list->selectedName().isNotEmpty() ? list->selectedName() : String ("Shared sequence"));
        askText ("Share sequence", "Copy this code and send it. Import reads it back.", code, "Copy", true, [code] (String) { SystemClipboard::copyTextToClipboard (code); });
    }
    void importCode()
    {
        const auto clip_ = SystemClipboard::getTextFromClipboard().trim();
        askText ("Import sequence", "Paste a sequence code (starts with PO-SEQ:)", clip_.startsWith ("PO-SEQ:") ? clip_ : String(), "Import", false, [this] (String code)
        { if (proc.importSequenceCode (code, {})) list->rebuild(); else say ("Import sequence", "That code was not recognised."); });
    }
    int page = 0, lastPh = -1; bool followPlay = true, ledOn = false, haveClip = false;
    std::array<uint16_t, 64> clip {}; SeqLanes* lanes = nullptr; SeqList* list = nullptr; TextBtn* play = nullptr; std::atomic<float>* mode_ = nullptr;
};

//==============================================================================
// FX: analyzer on the right, module rail on the left
class Analyzer : public Component
{
public:
    Analyzer (PurrOneProcessor& p, std::function<int()> mode) : proc (p), getMode (std::move (mode)), fft (11), win ((size_t) 2048)
    {
        setBounds (rc (L2::fxColour_items[2].r)); setInterceptsMouseClicks (false, false);
        for (int i = 0; i < 2048; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * MathConstants<float>::pi * (float) i / 2047.0f);
        std::fill (std::begin (sm), std::end (sm), -100.0f);
    }
    void grab (float* l, float* r, int n)
    {
        const int e = proc.tapPos.load(); for (int i = 0; i < n; ++i) { const int idx = (e - n + i + PurrOneProcessor::kTap * 2) % PurrOneProcessor::kTap; l[i] = proc.tapL[idx]; r[i] = proc.tapR[idx]; }
    }
    void tick() { repaint(); }
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        g.setColour (col::black); g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f); g.setColour (col::line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
        std::vector<float> l (2048), r (2048); grab (l.data(), r.data(), 2048);
        const int m = getMode();
        if (m == 0) paintSpectrum (g, w, h, l, r); else if (m == 1) paintScope (g, w, h, l, r); else paintGonio (g, getLocalBounds().toFloat(), l, r);
    }
    static float correlation (const std::vector<float>& l, const std::vector<float>& r)
    {
        double a = 0, b = 0, c = 0; for (size_t i = 0; i < l.size(); ++i) { a += l[i] * r[i]; b += l[i] * l[i]; c += r[i] * r[i]; }
        return b > 1.0e-9 && c > 1.0e-9 ? (float) (a / std::sqrt (b * c)) : 0.0f;
    }
    static void paintGonio (Graphics& g, Rectangle<float> a, const std::vector<float>& l, const std::vector<float>& r)
    {
        const float cx = a.getCentreX(), cy = a.getCentreY(), rad = jmin (a.getWidth(), a.getHeight()) * 0.46f;
        g.setColour (Colour (0xff1d1f26)); g.drawEllipse (cx - rad, cy - rad, rad * 2, rad * 2, 1.0f); g.drawLine (cx - rad, cy, cx + rad, cy); g.drawLine (cx, cy - rad, cx, cy + rad);
        g.setColour (Colour (0xff6f7480)); g.setFont (monoFont (10.0f)); g.drawText ("L", (int) (cx - rad), (int) (cy - rad - 14), 14, 12, Justification::centred, false); g.drawText ("R", (int) (cx + rad - 14), (int) (cy - rad - 14), 14, 12, Justification::centred, false);
        g.drawText ("M", (int) cx - 7, (int) (cy - rad - 14), 14, 12, Justification::centred, false);
        for (size_t i = 0; i < l.size(); i += 2)
        {
            const float mid = (l[i] + r[i]) * 0.7071f, side = (l[i] - r[i]) * 0.7071f;
            const float x = cx + juce::jlimit (-1.0f, 1.0f, side) * rad, y = cy - juce::jlimit (-1.0f, 1.0f, mid) * rad;
            g.setColour (col::green.withAlpha (0.55f)); g.fillRect (x - 1.0f, y - 1.0f, 2.0f, 2.0f);
        }
        const float c = correlation (l, r);
        g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText ("correlation " + String (c, 2) + (c > 0.9f ? "  (near mono)" : c < 0.0f ? "  (phase problem)" : ""), a.toNearestInt().removeFromBottom (16).reduced (6, 0), Justification::centredLeft, false);
    }
private:
    void paintSpectrum (Graphics& g, float w, float h, const std::vector<float>& l, const std::vector<float>& r)
    {
        float data[4096] = {}; for (int i = 0; i < 2048; ++i) data[i] = (l[(size_t) i] + r[(size_t) i]) * 0.5f * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (data);
        const double sr = jmax (8000.0, proc.getSampleRate());
        g.setFont (monoFont (9.0f));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        { const float x = std::log (f / 20.0f) / std::log (1000.0f) * w; g.setColour (Colour (0xff1d1f26)); g.drawVerticalLine ((int) x, 0.0f, h); g.setColour (Colour (0xff6f7480)); g.drawText (f >= 1000 ? String (f / 1000, 0) + "k" : String ((int) f), (int) x + 2, (int) h - 12, 30, 10, Justification::left, false); }
        for (float db : { -20.0f, -40.0f, -60.0f }) { g.setColour (Colour (0xff1d1f26)); g.drawHorizontalLine ((int) ((db / -90.0f) * h), 0.0f, w); }
        Path p; float peakDb = -100.0f, peakHz = 0.0f; const int cols = 120;
        for (int c = 0; c < cols; ++c)
        {
            const float f0 = 20.0f * std::pow (1000.0f, (float) c / cols), f1 = 20.0f * std::pow (1000.0f, (float) (c + 1) / cols);
            const int b0 = jlimit (1, 1023, (int) std::floor (f0 / (sr / 2048.0))), b1 = jlimit (b0, 1023, (int) std::ceil (f1 / (sr / 2048.0)));
            float mx = 0.0f; for (int b = b0; b <= b1; ++b) mx = jmax (mx, data[b]);
            const float db = 20.0f * std::log10 (mx / 512.0f + 1.0e-6f);
            sm[c] = db > sm[c] ? db : sm[c] * 0.85f + db * 0.15f;
            if (sm[c] > peakDb) { peakDb = sm[c]; peakHz = f0; }
            const float x = (float) c / (cols - 1) * w, y = jlimit (0.0f, h, (jmin (0.0f, sm[c]) / -90.0f) * h);
            if (c) p.lineTo (x, y); else p.startNewSubPath (x, y);
        }
        Path fill (p); fill.lineTo (w, h); fill.lineTo (0, h); fill.closeSubPath();
        g.setColour (col::green.withAlpha (0.14f)); g.fillPath (fill); g.setColour (col::green); g.strokePath (p, PathStrokeType (1.5f));
        g.setColour (col::inkDim); g.drawText ("peak " + (peakDb > -80.0f ? String (roundToInt (peakHz)) + " Hz" : String ("-")), 6, 2, 150, 12, Justification::left, false);
    }
    void paintScope (Graphics& g, float w, float h, const std::vector<float>& l, const std::vector<float>& r)
    {
        g.setColour (Colour (0xff1d1f26)); g.drawHorizontalLine ((int) (h / 2), 0.0f, w);
        int s = 0; for (int i = 1; i < 1024; ++i) if (l[(size_t) i - 1] < 0.0f && l[(size_t) i] >= 0.0f) { s = i; break; }
        for (int ch = 0; ch < 2; ++ch)
        {
            Path p; const auto& d = ch ? r : l;
            for (int i = 0; i < 1000; ++i) { const float x = (float) i / 999.0f * w, y = h / 2 - jlimit (-1.0f, 1.0f, d[(size_t) (s + i)]) * (h / 2 - 4); if (i) p.lineTo (x, y); else p.startNewSubPath (x, y); }
            g.setColour (ch ? col::blue.withAlpha (0.8f) : col::green); g.strokePath (p, PathStrokeType (1.5f));
        }
        g.setColour (col::inkDim); g.setFont (monoFont (10.0f)); g.drawText ("L green, R blue", 6, 2, 150, 12, Justification::left, false);
    }
    PurrOneProcessor& proc; std::function<int()> getMode; dsp::FFT fft; std::vector<float> win; float sm[120];
};

class CurveView : public Component   // transfer curve and a sine through it
{
public:
    CurveView (PurrOneProcessor& p) : proc (p) { setBounds (rc (L2::fxColour_items[1].r)); setInterceptsMouseClicks (false, false); }
    float v (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        const float d = v (ids::drive), x_ = v (ids::xstr), fz = v (ids::fuzz);
        const float mk = 1.0f; (void) mk;
        const float sq = h - 20.0f, ox = 10.0f, oy = 10.0f;
        g.setColour (Colour (0xff1a1c22)); g.drawRect (ox, oy, sq, sq, 1.0f); g.drawLine (ox, oy + sq / 2, ox + sq, oy + sq / 2); g.drawLine (ox + sq / 2, oy, ox + sq / 2, oy + sq);
        g.setColour (Colour (0xff3a3d48)); g.drawLine (ox, oy + sq, ox + sq, oy, 1.0f);
        Path p; for (int i = 0; i <= 200; ++i) { const float x = (float) i / 100.0f - 1.0f; const float y = jlimit (-1.0f, 1.0f, purrone::Engine::colourCurve (x, d, x_, fz)); const float px = ox + (x + 1) / 2 * sq, py = oy + (1 - (y + 1) / 2) * sq; if (i) p.lineTo (px, py); else p.startNewSubPath (px, py); }
        g.setColour (col::green); g.strokePath (p, PathStrokeType (2.0f));
        const float wx = ox + sq + 30.0f, ww = w - wx - 10.0f;
        g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText ("in", (int) wx, 8, 40, 14, Justification::left, false); g.drawText ("out", (int) wx, (int) (oy + sq / 2 + 8), 40, 14, Justification::left, false);
        for (int pass = 0; pass < 2; ++pass)
        {
            Path s; const float cy = oy + sq * (pass ? 0.74f : 0.26f), amp = sq * 0.19f;
            for (int k = 0; k <= 240; ++k) { const float t = (float) k / 240.0f, x2 = std::sin (t * 12.566f) * 0.9f; const float val = pass ? jlimit (-1.0f, 1.0f, purrone::Engine::colourCurve (x2, d, x_, fz)) : x2; const float px = wx + t * ww, py = cy - val * amp; if (k) s.lineTo (px, py); else s.startNewSubPath (px, py); }
            g.setColour (pass ? col::yellow : col::blue); g.strokePath (s, PathStrokeType (1.5f));
        }
        if (d + x_ + fz < 0.001f) { g.setColour (col::inkDim); g.drawText ("Colour is off: the curve is a straight line", (int) wx, (int) (h - 24), (int) ww, 14, Justification::left, false); }
    }
    void tick() { const float s = v (ids::drive) * 7.0f + v (ids::xstr) * 131.0f + v (ids::fuzz) * 977.0f; if (s != last) { last = s; repaint(); } }
private:
    PurrOneProcessor& proc; float last = -1;
};

class TailView : public Component
{
public:
    TailView (PurrOneProcessor& p) : proc (p) { setBounds (rc (L2::fxSpace_items[2].r)); setInterceptsMouseClicks (false, false); }
    float v (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        const float pre = v (ids::spPre) / 1000.0f, len = v (ids::verbSize), mix = v (ids::verbMix), damp = v (ids::spDamp);
        const float total = pre + len * 1.5f; auto X = [&] (float t) { return 10.0f + t / total * (w - 20.0f); };
        g.setColour (Colour (0xff4a4d58)); g.drawVerticalLine ((int) X (pre), 8.0f, h - 10.0f);
        Path p; p.startNewSubPath (X (0), h - 10); p.lineTo (X (pre), h - 10);
        for (int i = 0; i <= 120; ++i) { const float t = pre + len * (float) i / 120.0f * 1.2f; const float val = mix * std::exp (-6.9f * (t - pre) / len) * (1.0f - damp * 0.3f * ((float) i / 120.0f)); p.lineTo (X (t), h - 10 - jlimit (0.0f, 1.0f, val * 4.0f) * (h - 30)); }
        Path f (p); f.lineTo (X (pre + len * 1.2f), h - 10); g.setColour (col::green.withAlpha (0.12f)); g.fillPath (f); g.setColour (col::green); g.strokePath (p, PathStrokeType (2.0f));
        g.setColour (col::inkDim); g.setFont (monoFont (11.0f));
        g.drawText ("pre-delay " + String (roundToInt (v (ids::spPre))) + " ms   length " + String (len, 1) + " s   mix " + String (roundToInt (mix * 100.0f)) + "%", 10, 2, (int) w - 20, 14, Justification::left, false);
    }
    void tick() { const float s = v (ids::spPre) + v (ids::verbSize) * 31.0f + v (ids::verbMix) * 977.0f + v (ids::spDamp) * 7.0f; if (s != last) { last = s; repaint(); } }
private:
    PurrOneProcessor& proc; float last = -1;
};

class LevelView : public Component   // output level history
{
public:
    LevelView (PurrOneProcessor& p) : proc (p) { setBounds (rc (L2::fxDyn_items[1].r)); setInterceptsMouseClicks (false, false); }
    void tick() { const float l = jmax (proc.meterL.load(), proc.meterR.load()); hist.push_back (l); if (hist.size() > 180) hist.erase (hist.begin()); repaint(); }
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        g.setColour (Colour (0xff1a1c22)); for (float db : { -6.0f, -12.0f, -24.0f, -48.0f }) g.drawHorizontalLine ((int) (4 + db / -60.0f * (h - 24)), 0.0f, w);
        Path p; for (size_t i = 0; i < hist.size(); ++i) { const float db = hist[i] > 1.0e-5f ? 20.0f * std::log10 (hist[i]) : -90.0f; const float x = (float) i / 179.0f * w, y = 4 + jlimit (0.0f, 60.0f, -db) / 60.0f * (h - 24); if (i) p.lineTo (x, y); else p.startNewSubPath (x, y); }
        g.setColour (col::orange); g.strokePath (p, PathStrokeType (1.5f));
        g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText ("output level, last seconds (0 dB at the top)", 6, (int) h - 18, (int) w - 12, 14, Justification::left, false);
    }
private:
    PurrOneProcessor& proc; std::vector<float> hist;
};

class FieldView : public Component
{
public:
    FieldView (PurrOneProcessor& p) : proc (p), an (p, [] { return 2; }) { setBounds (rc (L2::fxStereo_items[1].r)); setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        std::vector<float> l (2048), r (2048); an.grab (l.data(), r.data(), 2048);
        Analyzer::paintGonio (g, getLocalBounds().toFloat(), l, r);
    }
    void tick() { repaint(); }
private:
    PurrOneProcessor& proc; Analyzer an;
};

// graphic EQ: five draggable points
struct EqBand { const char* f; const char* g; const char* q; const char* d; const char* t; const char* m; const char* o; };
inline const EqBand kEqBands[5] = {
    { ids::eqLoF, ids::eqLo, nullptr, ids::eqD1, ids::eqT1, ids::eqM1, ids::eqO1 },
    { ids::eqF2, ids::eqG2, ids::eqQ2, ids::eqD2, ids::eqT2, ids::eqM2, ids::eqO2 },
    { ids::eqMidF, ids::eqMid, ids::eqMidQ, ids::eqD3, ids::eqT3, ids::eqM3, ids::eqO3 },
    { ids::eqF4, ids::eqG4, ids::eqQ4, ids::eqD4, ids::eqT4, ids::eqM4, ids::eqO4 },
    { ids::eqHiF, ids::eqHi, nullptr, ids::eqD5, ids::eqT5, ids::eqM5, ids::eqO5 } };

class EqGraph : public Component
{
public:
    EqGraph (PurrOneProcessor& p, int& bandRef) : proc (p), band (bandRef) { setBounds (rc (L2::fxEq_items[2].r)); std::fill (std::begin (sm), std::end (sm), -100.0f); }
    float raw (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    float xOf (float f) const { return std::log (f / 20.0f) / std::log (1000.0f) * (float) getWidth(); }
    float yOf (float db) const { return (float) getHeight() * 0.5f - db / 15.0f * ((float) getHeight() * 0.5f - 16.0f); }
    static double mag (int type, double f0, double gDb, double q, double f)
    {
        const double fs = 48000.0, A = std::pow (10.0, gDb / 40.0), w0 = 2.0 * MathConstants<double>::pi * f0 / fs, cs = std::cos (w0), sn = std::sin (w0), al = sn / (2.0 * q);
        double b0, b1, b2, a0, a1, a2;
        if (type == 1) { b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A; }
        else { const double be = 2.0 * std::sqrt (A) * al;
               if (type == 0) { b0 = A * ((A + 1) - (A - 1) * cs + be); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - be); a0 = (A + 1) + (A - 1) * cs + be; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - be; }
               else { b0 = A * ((A + 1) + (A - 1) * cs + be); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - be); a0 = (A + 1) - (A - 1) * cs + be; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - be; } }
        const double w = 2.0 * MathConstants<double>::pi * f / fs; const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
        return std::abs ((b0 + b1 * z1 + b2 * z2) / (a0 + a1 * z1 + a2 * z2));
    }
    float bandDb (int b, float f, float extra = 0.0f) const
    {
        const auto& d = kEqBands[b]; if (raw (d.o) < 0.5f) return 0.0f;
        return (float) (20.0 * std::log10 (mag (b == 0 ? 0 : b == 4 ? 2 : 1, raw (d.f), jlimit (-24.0f, 24.0f, raw (d.g) + extra), d.q ? raw (d.q) : 0.7071, f)));
    }
    bool isDyn (int b) const { const auto& d = kEqBands[b]; return raw (d.o) > 0.5f && std::fabs (raw (d.d)) > 0.05f; }
    float liveExtra (int b) const { return isDyn (b) ? proc.eqView().liveDynDb (b) : 0.0f; }
    // spectrum scale is the same as the analyzer: 0 dBFS at the top, -90 at the bottom
    float ySpec (float db) const { return jlimit (0.0f, (float) getHeight(), db / -90.0f * (float) getHeight()); }
    void tick() { repaint(); }
    void paintSpectrum (Graphics& g, float w, float h)
    {
        std::vector<float> l (2048), r (2048);
        { const int e = proc.tapPos.load(); for (int i = 0; i < 2048; ++i) { const int idx = (e - 2048 + i + PurrOneProcessor::kTap * 2) % PurrOneProcessor::kTap; l[(size_t) i] = proc.tapL[idx]; r[(size_t) i] = proc.tapR[idx]; } }
        float data[4096] = {}; for (int i = 0; i < 2048; ++i) data[i] = (l[(size_t) i] + r[(size_t) i]) * 0.5f * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (data);
        const double sr = jmax (8000.0, proc.getSampleRate()); const int cols = 160; Path p;
        for (int c = 0; c < cols; ++c)
        {
            const float f0 = 20.0f * std::pow (1000.0f, (float) c / cols), f1 = 20.0f * std::pow (1000.0f, (float) (c + 1) / cols);
            const int b0 = jlimit (1, 1023, (int) std::floor (f0 / (sr / 2048.0))), b1 = jlimit (b0, 1023, (int) std::ceil (f1 / (sr / 2048.0)));
            float mx = 0.0f; for (int bb = b0; bb <= b1; ++bb) mx = jmax (mx, data[bb]);
            const float db = 20.0f * std::log10 (mx / 512.0f + 1.0e-6f);
            sm[c] = db > sm[c] ? db : sm[c] * 0.85f + db * 0.15f;
            const float x = (float) c / (cols - 1) * w, y = ySpec (jmin (0.0f, sm[c]));
            if (c) p.lineTo (x, y); else p.startNewSubPath (x, y);
        }
        Path fill (p); fill.lineTo (w, h); fill.lineTo (0, h); fill.closeSubPath();
        g.setColour (Colour (0xff8a93a6).withAlpha (0.13f)); g.fillPath (fill); g.setColour (Colour (0xff8a93a6).withAlpha (0.45f)); g.strokePath (p, PathStrokeType (1.0f));
    }
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        g.setFont (monoFont (10.0f));
        for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f }) { g.setColour (db == 0.0f ? Colour (0xff2b2d34) : Colour (0xff1d1f26)); g.drawHorizontalLine ((int) yOf (db), 0.0f, w); g.setColour (Colour (0xff6f7480)); g.drawText (String (db > 0 ? "+" : "") + String ((int) db), 4, (int) yOf (db) - 12, 30, 10, Justification::left, false); }
        for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f }) { const float x = xOf (f); g.setColour (Colour (0xff1d1f26)); g.drawVerticalLine ((int) x, 0.0f, h); g.setColour (Colour (0xff6f7480)); g.drawText (f >= 1000 ? String (f / 1000, 0) + "k" : String ((int) f), (int) x + 2, (int) h - 13, 30, 10, Justification::left, false); }
        paintSpectrum (g, w, h);
        // levels scale for the spectrum and the thresholds (dBFS), on the right edge
        g.setColour (Colour (0xff6f7480)); g.setFont (monoFont (9.0f));
        for (float db : { -20.0f, -40.0f, -60.0f }) g.drawText (String ((int) db) + " dBFS", (int) w - 62, (int) ySpec (db) - 11, 58, 10, Justification::right, false);

        // static curve (dim) and live curve (bright): the live one includes what the dynamic bands are doing right now
        Path stat, live, ghost; const int n = 300; bool anyDyn = false; for (int b = 0; b < 5; ++b) anyDyn |= isDyn (b);
        for (int i = 0; i < n; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / (n - 1)); float ds = 0, dl = 0, dg = 0;
            for (int b = 0; b < 5; ++b)
            {
                ds += bandDb (b, f); dl += bandDb (b, f, liveExtra (b));
                dg += bandDb (b, f, b == band && isDyn (b) ? raw (kEqBands[b].d) : liveExtra (b));
            }
            const float x = (float) i / (n - 1) * w;
            const float ys = yOf (jlimit (-15.0f, 15.0f, ds)), yl = yOf (jlimit (-15.0f, 15.0f, dl)), yg = yOf (jlimit (-15.0f, 15.0f, dg));
            if (i) { stat.lineTo (x, ys); live.lineTo (x, yl); ghost.lineTo (x, yg); } else { stat.startNewSubPath (x, ys); live.startNewSubPath (x, yl); ghost.startNewSubPath (x, yg); }
        }
        if (anyDyn) { g.setColour (Colour (0xff9aa0ad).withAlpha (0.6f)); g.strokePath (stat, PathStrokeType (1.0f)); }
        if (anyDyn && isDyn (band)) { Path dashed; const float dl[2] = { 5.0f, 4.0f }; PathStrokeType (1.5f).createDashedStroke (dashed, ghost, dl, 2); g.setColour (col::ink.withAlpha (0.75f)); g.fillPath (dashed); }
        Path fill (live); fill.lineTo (w, yOf (0)); fill.lineTo (0, yOf (0)); fill.closeSubPath(); g.setColour (col::yellow.withAlpha (0.10f)); g.fillPath (fill); g.setColour (col::yellow); g.strokePath (live, PathStrokeType (2.0f));

        for (int b = 0; b < 5; ++b)
        {
            const auto& d = kEqBands[b]; const int tg = (int) std::lround (raw (d.m)); const Colour c = tg == 0 ? col::green : tg == 1 ? col::blue : col::orange;
            const bool on = raw (d.o) > 0.5f; const float x = xOf (raw (d.f));
            if (isDyn (b))
            {
                // threshold line across the band, and the live detector level inside the band
                const float q = d.q ? raw (d.q) : 0.7f, fc = raw (d.f), sp = std::pow (2.0f, 0.72f / jmax (0.3f, q));
                const float xa = b == 0 ? 0.0f : xOf (jmax (20.0f, fc / sp)), xb = b == 4 ? w : xOf (jmin (20000.0f, fc * sp));
                const float yt = ySpec (raw (d.t)), lv = proc.eqView().liveLevelDb (b), yv = ySpec (jmin (0.0f, lv));
                const bool over = lv > raw (d.t);
                g.setColour (c.withAlpha (b == band ? 0.95f : 0.5f)); g.drawLine (xa, yt, xb, yt, b == band ? 2.0f : 1.0f);
                g.drawLine (xa, yt - 4.0f, xa, yt + 4.0f, 1.0f); g.drawLine (xb, yt - 4.0f, xb, yt + 4.0f, 1.0f);
                g.setColour (over ? col::pink : c); g.fillRect (x - 3.0f, yv - 3.0f, 6.0f, 6.0f);
                if (over) { g.setColour (col::pink.withAlpha (0.7f)); g.drawLine (x, yt, x, yv, 2.0f); }
                if (b == band) { g.setColour (c); g.setFont (monoFont (10.0f)); g.drawText ("threshold " + String (roundToInt (raw (d.t))) + " dB", (int) xa + 4, (int) yt - 14, 130, 11, Justification::left, false); }
            }
            const float live0 = raw (d.g) + liveExtra (b), y = yOf (jlimit (-15.0f, 15.0f, raw (d.g)));
            if (isDyn (b))
            {
                const float yl = yOf (jlimit (-15.0f, 15.0f, live0)), ym = yOf (jlimit (-15.0f, 15.0f, raw (d.g) + raw (d.d)));
                g.setColour (c.withAlpha (0.5f)); g.drawLine (x, y, x, ym, 1.0f); g.drawEllipse (x - 5.0f, ym - 5.0f, 10.0f, 10.0f, 1.5f);   // hollow: where it would end up at full effect
                g.setColour (c); g.fillEllipse (x - 4.0f, yl - 4.0f, 8.0f, 8.0f);                                                             // small solid: where it is now
            }
            g.setColour (c.withAlpha (on ? 1.0f : 0.3f)); g.fillEllipse (x - 8.0f, y - 8.0f, 16.0f, 16.0f);
            if (b == band) { g.setColour (col::ink); g.drawEllipse (x - 11.0f, y - 11.0f, 22.0f, 22.0f, 2.0f); }
            g.setColour (col::black); g.setFont (labelFont (12.0f, true, 0.0f)); g.drawText (String (b + 1), Rectangle<float> (x - 8.0f, y - 8.0f, 16.0f, 16.0f).toNearestInt(), Justification::centred, false);
        }
        g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText ("green: stereo   blue: mid   orange: side   grey: output spectrum", 40, 4, 600, 14, Justification::left, false);
        if (isDyn (band))
        {
            const auto& d = kEqBands[band]; const float now = proc.eqView().liveDynDb (band);
            g.setColour (col::ink); g.drawText ("band " + String (band + 1) + " dynamic: now " + String (now > 0 ? "+" : "") + String (now, 1) + " dB, full " + String (raw (d.d) > 0 ? "+" : "") + String (raw (d.d), 1) + " dB (hollow dot, dashed line)", 40, 20, 520, 14, Justification::left, false);
        }
    }
    int nearest (Point<int> pt) const
    {
        int best = -1; float bd = 18.0f;
        for (int b = 0; b < 5; ++b) { const auto& d = kEqBands[b]; const float dx = xOf (raw (d.f)) - pt.x, dy = yOf (jlimit (-15.0f, 15.0f, raw (d.g))) - pt.y; const float dist = std::sqrt (dx * dx + dy * dy); if (dist < bd) { bd = dist; best = b; } }
        return best;
    }
    void mouseDown (const MouseEvent& e) override
    {
        const int b = nearest (e.getPosition()); if (b < 0) return;
        band = b; drag = b; const auto& d = kEqBands[b]; fp = proc.apvts.getParameter (d.f); gp = proc.apvts.getParameter (d.g);
        if (fp) fp->beginChangeGesture(); if (gp) gp->beginChangeGesture();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (drag < 0) return;
        const float f = 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, (float) e.x / (float) getWidth()));
        const auto rng = fp->getNormalisableRange();
        setReal (fp, jlimit (rng.start, rng.end, f));
        setReal (gp, jlimit (-12.0f, 12.0f, (float) ((getHeight() * 0.5 - e.y) / (getHeight() * 0.5 - 16.0) * 15.0)));
    }
    void mouseUp (const MouseEvent&) override { if (drag >= 0) { if (fp) fp->endChangeGesture(); if (gp) gp->endChangeGesture(); } drag = -1; }
    void mouseDoubleClick (const MouseEvent& e) override { const int b = nearest (e.getPosition()); if (b >= 0) { band = b; if (auto* p = proc.apvts.getParameter (kEqBands[b].g)) p->setValueNotifyingHost (p->getDefaultValue()); } }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        int b = nearest (e.getPosition()); if (b < 0) b = band; if (kEqBands[b].q == nullptr || w.deltaY == 0.0f) return;
        if (auto* q = proc.apvts.getParameter (kEqBands[b].q)) { band = b; setReal (q, jlimit (0.3f, 4.0f, realOf (q) + (w.deltaY > 0 ? 0.1f : -0.1f))); }
    }
private:
    PurrOneProcessor& proc; int& band; int drag = -1; RangedAudioParameter *fp = nullptr, *gp = nullptr; float last = -1;
    dsp::FFT fft { 11 }; std::vector<float> win = [] { std::vector<float> w (2048); for (int i = 0; i < 2048; ++i) w[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * MathConstants<float>::pi * (float) i / 2047.0f); return w; }(); float sm[160];
};

class FxPage : public PageBase
{
public:
    FxPage (PurrOneProcessor& p, Bubbles& b, int viewIdx, Analyzer& an, std::function<int()> anaMode, std::function<void(int)> setAna)
        : PageBase (p, b, L2::views[viewIdx]), idx (viewIdx), analyzer (an), getAna (anaMode), setAna_ (setAna)
    {
        const bool eq = viewIdx == L2::vFxEq;
        build ([eq] (const char* id) { const String s (id); if (s == "anaTabs" || s == "bandSel") return true; if (! eq) return false; return s.startsWith ("eq"); });   // every EQ control is built per band in buildEq()
        addLocal ("anaTabs", getAna, setAna_);
        if (viewIdx == L2::vFxColour) curve = add (new CurveView (proc));
        if (viewIdx == L2::vFxSpace)  tail = add (new TailView (proc));
        if (viewIdx == L2::vFxDyn)    level = add (new LevelView (proc));
        if (viewIdx == L2::vFxStereo) field = add (new FieldView (proc));
        if (eq) buildEq();
    }
    void buildEq()
    {
        graph = add (new EqGraph (proc, band));
        addLocal ("bandSel", [this] { return band; }, [this] (int v) { band = v; layoutBand(); });
        static const char* names[5] = { "LOW SHELF", "LOW MID", "MID", "HIGH MID", "HIGH SHELF" };
        (void) names;
        auto& a = proc.apvts;
        for (int b = 0; b < 5; ++b)
        {
            const auto& d = kEqBands[b];
            auto mk = [&] (const char* id, const char* label, int slot, char fmt, const char* tip) -> Knob*
            {
                auto* def = new layout::Knob { id, label, { 241 + 70 * slot, 516, 62, 74 }, { 250 + 70 * slot, 530, 44, 44 }, fmt, tip }; defs.add (def);
                auto* k = new Knob (*a.getParameter (id), *def, bubbles); owned.add (k); addAndMakeVisible (k); bandWidgets[b].add (k); return k;
            };
            int slot = 0;
            mk (d.f, "FREQ", slot++, 'C', "Where the band acts.");
            mk (d.g, b == 0 || b == 4 ? "GAIN" : "GAIN", slot++, 'D', b == 0 ? "Low shelf. A small cut clears mud; a small boost adds weight to a bass." : b == 4 ? "High shelf. Adds air, or tames fizz." : "Boost or cut around the frequency.");
            if (d.q) mk (d.q, "Q", slot++, 'Q', "Width of the band. The mouse wheel over a point changes it too. Higher is narrower.");
            mk (d.d, "DYNAMIC", slot++, 'Z', "Makes the band move with the level. 0 keeps it static. Positive boosts and negative cuts, by up to that many dB, when the signal in the band is above the threshold. The display shows the result live.");
            mk (d.t, "THRESHOLD", slot++, 'K', "Level above which the dynamic part starts working. The line on the graph shows where it sits against the spectrum; the square is the live level in the band.");
            auto* tg = new layout::Tog { d.o, "ON", { 415, 482, 56, 26 }, 0xff02ebbau, "Switches this band on or off." }; tgDefs.add (tg);
            auto* lt = new LedToggle (*a.getParameter (d.o), *tg, bubbles); owned.add (lt); addAndMakeVisible (lt); bandWidgets[b].add (lt);
            auto* sd = segDef ("eqM1");
            auto* ts = new Segments (*a.getParameter (d.m), *sd, bubbles, L2::segBtns2); owned.add (ts); addAndMakeVisible (ts); bandWidgets[b].add (ts);
        }
        layoutBand();
    }
    void layoutBand() { for (int b = 0; b < 5; ++b) for (auto* c : bandWidgets[b]) c->setVisible (b == band); repaint(); }
    String secTitle (int i) const override
    {
        if (idx == L2::vFxEq && i == 1) { static const char* n[5] = { "LOW SHELF", "LOW MID", "MID", "HIGH MID", "HIGH SHELF" }; return "BAND " + String (band + 1) + " " + n[band]; }
        return PageBase::secTitle (i);
    }
    String secSub (int i) const override
    {
        if (idx == L2::vFxEq && i == 1) { const auto& d = kEqBands[band]; auto& a = proc.apvts; const float f = a.getRawParameterValue (d.f)->load(), g = a.getRawParameterValue (d.g)->load();
            return String (f >= 1000 ? String (f / 1000.0f, 1) + " KHZ" : String (roundToInt (f)) + " HZ") + "  " + String (g > 0 ? "+" : "") + String (g, 1) + " DB"; }
        return PageBase::secSub (i);
    }
    bool showNote (const String&) const override { return true; }
    void tick() override
    {
        PageBase::tick();
        if (idx == L2::vFxEq && shownBand != band) { shownBand = band; layoutBand(); }   // a band picked by clicking its point in the graph
        if (curve) curve->tick(); if (tail) tail->tick(); if (level) level->tick(); if (field) field->tick(); if (graph) graph->tick();
        if (idx == L2::vFxEq) { const int sig = band * 100000 + (int) (proc.apvts.getRawParameterValue (kEqBands[band].f)->load() * 10) + (int) (proc.apvts.getRawParameterValue (kEqBands[band].g)->load() * 100); if (sig != lastSig) { lastSig = sig; repaint(); } }
    }
    int idx; int band = 2, shownBand = 2;
private:
    Analyzer& analyzer; std::function<int()> getAna; std::function<void(int)> setAna_;
    CurveView* curve = nullptr; TailView* tail = nullptr; LevelView* level = nullptr; FieldView* field = nullptr; EqGraph* graph = nullptr;
    OwnedArray<layout::Knob> defs; OwnedArray<layout::Tog> tgDefs; Array<Component*> bandWidgets[5]; int lastSig = 0;
};

class FxHost : public Component
{
public:
    FxHost (PurrOneProcessor& p, Bubbles& b) : proc (p), bubbles (b), analyzer (p, [this] { return anaMode; })
    {
        setBounds (0, 0, layout::W, layout::H); setInterceptsMouseClicks (false, true);
        for (int i = 0; i < 5; ++i)
        {
            pages.add (new FxPage (p, b, L2::vFxColour + i, analyzer, [this] { return anaMode; }, [this] (int v) { anaMode = v; }));
            addChildComponent (pages.getLast());
        }
        addAndMakeVisible (analyzer);
        preset = new TextBtn ("Presets...", "Presets for the modules on this page. Each one only changes its own module."); addAndMakeVisible (preset);
        preset->setBounds (62, 446, 146, 31); preset->onClick = [this] { menu(); };
        select (0);
    }
    void testBand (int b) { if (auto* e = pages[1]) { e->band = jlimit (0, 4, b); e->layoutBand(); } }   // screenshot test hook
    void select (int i) { cur = i; for (int k = 0; k < pages.size(); ++k) pages[k]->setVisible (k == cur); repaint(); }
    int current() const { return cur; }
    void tick() { pages[cur]->tick(); analyzer.tick(); }
    void paint (Graphics& g) override
    {
        const auto& v = L2::fxColour_items;
        { // rail
            const auto r = rc (v[3].r).toFloat(); g.setColour (col::panel2); g.fillRoundedRectangle (r, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            g.setColour (Colour (0xff3a3d48)); g.fillRect (r.getX() + 1.0f, r.getY(), r.getWidth() - 2.0f, 3.0f);
        }
        for (int i = 0; i < 5; ++i)
        {
            const auto& it = v[4 + i]; auto r = rc (it.r).reduced (6, 3).toFloat(); const bool sel = i == cur;
            g.setColour (sel ? Colour (0xff1f3a33) : col::field); g.fillRoundedRectangle (r, 3.0f); g.setColour (sel ? Colour (0xff2c6a59) : col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            g.setColour (sel ? col::green : col::ink); g.setFont (labelFont (17.0f, true, 0.1f)); g.drawText (String (it.text).toUpperCase(), r.toNearestInt().withTrimmedLeft (12).withHeight (30), Justification::centredLeft, false);
            g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.04f)); g.drawText (it.sub, r.toNearestInt().withTrimmedLeft (12).withTrimmedTop (28), Justification::topLeft, false);
        }
        g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.04f)); g.drawFittedText ("Signal order is fixed: Colour, Filter, Amp, Crush, EQ, Space, Opto, OTT, Dance master, Maximizer, Stereo, Limiter.", rc (v[9].r).reduced (8, 0), Justification::topLeft, 5, 1.0f);
        { // analyzer panel
            const auto r = rc (v[10].r).toFloat(); g.setColour (col::panel2); g.fillRoundedRectangle (r, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            g.setColour (Colour (0xff3a3d48)); g.fillRect (r.getX() + 1.0f, r.getY(), r.getWidth() - 2.0f, 3.0f);
            g.setColour (col::ink); g.setFont (labelFont (16.0f, true, 0.2f)); g.drawText ("ANALYZER", (int) r.getX() + 11, (int) r.getY() + 8, 200, 18, Justification::centredLeft, false);
            g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.04f));
            g.drawFittedText ("Reads the final output. Spectrum and scope help you see mud, harshness and level while you shape the sound; Phase shows the stereo field. New modules (OTT, Dance master, Stereo, Crush) start off or neutral, so the sound stays exactly as it was until you use them.", Rectangle<int> (1331, 568, 308, 84), Justification::topLeft, 6, 1.0f);
        }
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < 5; ++i) if (rc (L2::fxColour_items[4 + i].r).contains (e.getPosition())) { select (i); return; }
    }
    bool hitTest (int x, int y) override { for (int i = 0; i < 5; ++i) if (rc (L2::fxColour_items[4 + i].r).contains (x, y)) return true; return Component::hitTest (x, y); }
    void menu()
    {
        static const std::vector<std::vector<String>> mods = { { "Colour", "Crush" }, { "EQ" }, { "Space" }, { "Opto", "OTT", "Dance master", "Maximizer" }, { "Stereo" } };
        PopupMenu m; int id = 1; std::vector<std::pair<const std::vector<std::pair<const char*, float>>*, int>> map;
        const auto& all = purrone::fxModules();
        for (auto& name : mods[(size_t) cur])
            for (auto& fm : all) if (name == fm.module)
            {
                PopupMenu sub; for (auto& pr : fm.presets) { sub.addItem (id++, pr.name); map.push_back ({ &pr.values, 0 }); }
                if (mods[(size_t) cur].size() > 1) m.addSubMenu (String (fm.module).toUpperCase(), sub); else m = sub;
            }
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (preset).withStandardItemHeight (26), [this, map] (int r) { if (r > 0 && r <= (int) map.size()) proc.applyValues (*map[(size_t) r - 1].first); });
    }
    PurrOneProcessor& proc; Bubbles& bubbles; int anaMode = 0, cur = 0; Analyzer analyzer; OwnedArray<FxPage> pages; TextBtn* preset = nullptr;
};

//==============================================================================
// Settings
class CentsBars : public Component, public SettableTooltipClient
{
public:
    CentsBars (PurrOneProcessor& p) : proc (p) { setBounds (rc (itemOf (L2::views[L2::vSet], "cents").r)); setTooltip ("Drag a bar to pull that note out of tune. Double click resets it."); }
    static const char* id (int i) { static const char* c[12] = { ids::tuneC0, ids::tuneC1, ids::tuneC2, ids::tuneC3, ids::tuneC4, ids::tuneC5, ids::tuneC6, ids::tuneC7, ids::tuneC8, ids::tuneC9, ids::tuneC10, ids::tuneC11 }; return c[i]; }
    int temp() const { return (int) std::lround (proc.apvts.getRawParameterValue (ids::tuneTemp)->load()); }
    float cents (int i) const
    {
        static const double just[12] = { 1, 16.0 / 15, 9.0 / 8, 6.0 / 5, 5.0 / 4, 4.0 / 3, 45.0 / 32, 3.0 / 2, 8.0 / 5, 5.0 / 3, 16.0 / 9, 15.0 / 8 };
        static const double pyth[12] = { 1, 256.0 / 243, 9.0 / 8, 32.0 / 27, 81.0 / 64, 4.0 / 3, 729.0 / 512, 3.0 / 2, 128.0 / 81, 27.0 / 16, 16.0 / 9, 243.0 / 128 };
        const int t = temp();
        if (t == 1) return (float) (1200.0 * std::log2 (just[i]) - 100.0 * i);
        if (t == 2) return (float) (1200.0 * std::log2 (pyth[i]) - 100.0 * i);
        if (t == 3) return proc.apvts.getRawParameterValue (id (i))->load();
        return 0.0f;
    }
    void tick() { float s = (float) temp(); for (int i = 0; i < 12; ++i) s += cents (i) * (float) (i + 1); if (s != last) { last = s; repaint(); } }
    void paint (Graphics& g) override
    {
        g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        static const char* nm[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const float w = (float) getWidth() / 12.0f, h = (float) getHeight(), mid = (h - 22.0f) * 0.5f + 4.0f;
        g.setColour (Colour (0xff2b2d34)); g.drawHorizontalLine ((int) mid, 0.0f, (float) getWidth());
        for (int i = 0; i < 12; ++i)
        {
            const float c = cents (i), x = i * w + w * 0.2f, bw = w * 0.6f, y = mid - c / 50.0f * (mid - 6.0f);
            g.setColour (temp() == 3 ? col::green : col::green.withAlpha (0.45f)); g.fillRect (x, jmin (y, mid), bw, jmax (2.0f, std::abs (y - mid)));
            g.setColour (col::ink); g.setFont (monoFont (11.0f)); g.drawText ((c > 0 ? "+" : "") + String (roundToInt (c)), (int) (i * w), (int) h - 34, (int) w, 12, Justification::centred, false);
            g.setColour (col::inkDim); g.setFont (labelFont (13.0f, true, 0.06f)); g.drawText (nm[i], (int) (i * w), (int) h - 20, (int) w, 16, Justification::centred, false);
        }
    }
    void mouseDown (const MouseEvent& e) override { startEdit(); set (e); }
    void mouseDrag (const MouseEvent& e) override { set (e); }
    void mouseDoubleClick (const MouseEvent& e) override { startEdit(); const int i = jlimit (0, 11, (int) ((float) e.x / ((float) getWidth() / 12.0f))); setReal (proc.apvts.getParameter (id (i)), 0.0f); }
private:
    void startEdit()   // from a fixed temperament: copy what is shown into the custom bars, then switch to Custom
    {
        if (temp() != 3) { for (int i = 0; i < 12; ++i) setReal (proc.apvts.getParameter (id (i)), jlimit (-50.0f, 50.0f, cents (i))); proc.setParam (ids::tuneTemp, 3.0f); }
    }
    void set (const MouseEvent& e)
    {
        const float w = (float) getWidth() / 12.0f; const int i = jlimit (0, 11, (int) ((float) e.x / w)); const float h = (float) getHeight(), mid = (h - 22.0f) * 0.5f + 4.0f;
        setReal (proc.apvts.getParameter (id (i)), jlimit (-50.0f, 50.0f, (mid - (float) e.y) / (mid - 6.0f) * 50.0f));
    }
    PurrOneProcessor& proc; float last = -1;
};

class BankList : public Component
{
public:
    BankList (PurrOneProcessor& p, int& bankRef) : proc (p), bank (bankRef) { setBounds (rc (itemOf (L2::views[L2::vSet], "list").r)); rebuild(); }
    void rebuild()
    {
        rows.clear(); const auto& f = purrone::factoryPresets();
        if (bank <= 2) { for (int i = bank * purrone::kBankSize; i < jmin ((int) f.size(), (bank + 1) * purrone::kBankSize); ++i) rows.add ({ f[(size_t) i].name, f[(size_t) i].category, i, true }); }
        else for (auto& n : proc.listBank (bank)) rows.add ({ n, "yours", -1, false });
        scroll = 0; repaint();
    }
    void paint (Graphics& g) override
    {
        g.fillAll (col::black); g.setColour (col::line); g.drawRect (getLocalBounds(), 1);
        const int vis = (getHeight() - 2) / 22;
        if (rows.isEmpty()) { g.setColour (col::inkDim); g.setFont (labelFont (14.0f, false, 0.05f)); g.drawText (bank <= 2 ? "Empty" : "Empty bank. Save a sound while this bank is selected, or import a bank.", getLocalBounds().reduced (12), Justification::centredLeft, true); return; }
        for (int i = scroll; i < jmin (rows.size(), scroll + vis); ++i)
        {
            const auto r = Rectangle<int> (1, 1 + (i - scroll) * 22, getWidth() - 2, 22); const bool cur = rows[i].name == proc.currentPresetName;
            if (cur) { g.setColour (Colour (0xff1f3a33)); g.fillRect (r); }
            g.setColour (cur ? col::green : col::ink); g.setFont (monoFont (13.0f)); g.drawText (String (i + 1).paddedLeft ('0', 3) + "  " + rows[i].name, r.reduced (8, 0), Justification::centredLeft, true);
            g.setColour (col::inkDim); g.setFont (monoFont (11.0f)); g.drawText (rows[i].cat, r.reduced (8, 0), Justification::centredRight, false);
        }
    }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = scroll + (e.y - 1) / 22; if (i < 0 || i >= rows.size()) return;
        if (rows[i].index >= 0) proc.setCurrentProgram (rows[i].index); else proc.loadUserPreset (rows[i].name);
        repaint();
    }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { scroll = jlimit (0, jmax (0, rows.size() - (getHeight() - 2) / 22), scroll + (w.deltaY > 0 ? -3 : 3)); repaint(); }
    int count() const { return rows.size(); }
    void tick() { if (proc.currentPresetName != lastName) { lastName = proc.currentPresetName; repaint(); } }
private:
    struct Row { String name, cat; int index; bool factory; };
    PurrOneProcessor& proc; int& bank; Array<Row> rows; int scroll = 0; String lastName;
};

class SettingsPage : public PageBase
{
public:
    SettingsPage (PurrOneProcessor& p, Bubbles& b, std::function<int()> getZoom, std::function<void(int)> setZoom) : PageBase (p, b, L2::views[L2::vSet])
    {
        build ([] (const char* id) { return String (id) == "winSize"; });
        addLocal ("winSize", getZoom, setZoom);
        cents = add (new CentsBars (proc));
        bank = proc.saveBank; list = add (new BankList (proc, bank));
        auto r = rc (itemOf (vd, "bankbtn").r);
        for (int i = 0; i < 7; ++i)
        {
            auto* t = add (new TextBtn (String::charToString ((juce_wchar) ('A' + i)), i <= 2 ? "Factory bank, read only." : "Your bank. Up to 128 sounds."));
            t->setBounds (r.getX() + (r.getWidth() * i) / 7, r.getY(), r.getWidth() / 7 - 6, r.getHeight()); t->onClick = [this, i] { selectBank (i); }; bankBtns.add (t);
        }
        selectBank (proc.saveBank);
        auto btn = [this] (const char* label, const char* tip, std::function<void()> fn) { auto* t = add (new TextBtn (label, tip)); t->setBounds (rc (itemOf (vd, "btn", label).r)); t->onClick = std::move (fn); };
        btn ("Import bank", "Reads a bank file into the selected bank (D to G). Existing sounds with the same name are kept.", [this] { importBank(); });
        btn ("Export bank", "Writes the selected bank (D to G) to a file you can share.", [this] { exportBank(); });
        btn ("Import sound", "Reads one sound file into the selected bank (D to G).", [this] { importSound(); });
        btn ("Export sound", "Writes the current sound to a file you can share.", [this] { exportSound(); });
    }
    void selectBank (int i) { bank = i; if (i >= 3) proc.saveBank = i; for (int k = 0; k < bankBtns.size(); ++k) bankBtns[k]->on = k == i, bankBtns[k]->repaint(); list->rebuild(); repaint(); }
    void tick() override { PageBase::tick(); cents->tick(); list->tick(); }
    bool showNote (const String& t) const override { return ! t.startsWith ("A bank is one file"); }
    void paint (Graphics& g) override
    {
        PageBase::paint (g);
        g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.04f));
        g.drawFittedText (String ("Bank ") + String::charToString ((juce_wchar) ('A' + bank)) + ": " + String (list->count()) + " of 128 sounds. "
                          + (bank <= 2 ? "Factory, read only. Your saves go to bank " + String::charToString ((juce_wchar) ('A' + proc.saveBank)) + ". " : String ("Your saves go to this bank. ")) + "One bank is one file you can share.",
                          Rectangle<int> (61, 377, 461, 40), Justification::topLeft, 3, 1.0f);
    }
private:
    void importBank()
    {
        const int target = bank >= 3 ? bank : proc.saveBank;
        chooser = std::make_unique<FileChooser> ("Import bank", File(), "*.purrbank"); chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, target] (const FileChooser& fc)
        { const auto f = fc.getResult(); if (f == File()) return; const int n = proc.importBank (f, target); if (n < 0) say ("Import bank", "That file is not a PurrOne bank."); else { say ("Import bank", String (n) + " sounds added to bank " + String::charToString ((juce_wchar) ('A' + target)) + "."); selectBank (target); } });
    }
    void exportBank()
    {
        if (bank <= 2) { say ("Export bank", "Factory banks are built into the plugin. Pick one of your banks D to G."); return; }
        const int b = bank;
        chooser = std::make_unique<FileChooser> ("Export bank", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (String ("PurrOne bank ") + String::charToString ((juce_wchar) ('A' + b)) + ".purrbank"), "*.purrbank");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this, b] (const FileChooser& fc)
        { auto f = fc.getResult(); if (f == File()) return; if (f.getFileExtension().isEmpty()) f = f.withFileExtension (".purrbank"); if (! proc.exportBank (b, f)) say ("Export bank", "This bank is empty, so there is nothing to export."); });
    }
    void importSound()
    {
        const int target = bank >= 3 ? bank : proc.saveBank;
        chooser = std::make_unique<FileChooser> ("Import sound", File(), "*.purrone"); chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, target] (const FileChooser& fc)
        { const auto f = fc.getResult(); if (f == File()) return; if (proc.importSound (f, target)) selectBank (target); else say ("Import sound", "That file could not be read, or the bank is full."); });
    }
    void exportSound()
    {
        chooser = std::make_unique<FileChooser> ("Export sound", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (File::createLegalFileName (proc.currentPresetName) + ".purrone"), "*.purrone");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
        { auto f = fc.getResult(); if (f == File()) return; if (f.getFileExtension().isEmpty()) f = f.withFileExtension (".purrone"); proc.exportSound (f); });
    }
    int bank = 3; BankList* list = nullptr; CentsBars* cents = nullptr; Array<TextBtn*> bankBtns; std::unique_ptr<FileChooser> chooser;
};

} // namespace purrone::ui
