#include "PluginEditor.h"
#include "Presets.h"

using namespace purrone::ui;
namespace L = purrone::layout;

namespace
{
const char* kLogoSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 64 64\">"
    "<path d=\"M4.5 5C10 7 17 11 21 14c3-.5 7-2 11-2s8 1.500 11 2C47 11 55 7 60 5.500L61 24C61 40 48 50 32 58 16 50 3 40 3.700 24z\" fill=\"#f4f5f7\"/>"
    "<path d=\"M11.300 25.800C20 26 25 31 27 36.500 21 37.500 15 36 12.500 31zM52.700 25.800C44 26 39 31 37 36.500 43 37.500 49 36 51.500 31z\" fill=\"#15161a\"/></svg>";
}

//==============================================================================
// Look: dark menus and tooltips that match the panel.
struct PurrOneEditor::Look : public juce::LookAndFeel_V4
{
    Look()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d0e11));
        setColour (juce::PopupMenu::textColourId, col::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::panel2);
        setColour (juce::PopupMenu::highlightedTextColourId, col::green);
        setColour (juce::PopupMenu::headerTextColourId, col::inkDim);
        setColour (juce::AlertWindow::backgroundColourId, col::panel2);
        setColour (juce::AlertWindow::textColourId, col::ink);
        setColour (juce::TextButton::buttonColourId, col::field);
        setColour (juce::TextButton::textColourOffId, col::ink);
        setColour (juce::TextEditor::backgroundColourId, col::black);
        setColour (juce::TextEditor::textColourId, col::ink);
        setColour (juce::TextEditor::outlineColourId, col::line);
        setColour (juce::TextEditor::focusedOutlineColourId, col::green);
        setColour (juce::TooltipWindow::backgroundColourId, col::black);
        setColour (juce::TooltipWindow::textColourId, col::ink);
        setColour (juce::TooltipWindow::outlineColourId, juce::Colour (0xff4a4d58));
    }
    juce::Font getPopupMenuFont() override { return monoFont (14.0f); }
    juce::Rectangle<int> getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> area) override
    {
        juce::TextLayout tl = layoutFor (text, 240.0f);
        const int w = (int) tl.getWidth() + 20, h = (int) tl.getHeight() + 14;
        return juce::Rectangle<int> (pos.x > area.getCentreX() ? pos.x - w - 12 : pos.x + 14, pos.y + 18, w, h).constrainedWithin (area);
    }
    void drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        g.setColour (col::black); g.fillRoundedRectangle (0, 0, (float) w, (float) h, 4.0f);
        g.setColour (juce::Colour (0xff4a4d58)); g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f, 4.0f, 1.0f);
        layoutFor (text, 240.0f).draw (g, juce::Rectangle<float> (10.0f, 7.0f, (float) w - 20.0f, (float) h - 14.0f));
    }
    static juce::TextLayout layoutFor (const juce::String& text, float maxW)
    {
        juce::AttributedString as; as.setJustification (juce::Justification::topLeft);
        const auto parts = juce::StringArray::fromLines (text);
        as.append (parts[0].toUpperCase() + (parts.size() > 1 ? "\n" : ""), labelFont (13.0f, true, 0.14f), col::green);
        if (parts.size() > 1) as.append (parts.joinIntoString (" ", 1), juce::Font (juce::FontOptions (13.0f)), col::ink);
        juce::TextLayout tl; tl.createLayout (as, maxW); return tl;
    }
    void drawCornerResizer (juce::Graphics& g, int w, int h, bool, bool) override
    {
        g.setColour (col::ink.withAlpha (0.75f));
        for (float o : { 5.0f, 11.0f }) g.drawLine ((float) w - o - 1.0f, (float) h - 1.0f, (float) w - 1.0f, (float) h - o - 1.0f, 1.5f);
    }
};

//==============================================================================
struct PurrOneEditor::Button : public juce::Component, public juce::SettableTooltipClient
{
    Button (const juce::String& t, const juce::String& tip = {}) : text (t) { setMouseCursor (juce::MouseCursor::PointingHandCursor); setTooltip (tip); }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (on ? juce::Colour (0xff1f3a33) : over ? juce::Colour (0xff23252c) : col::panel2); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (on ? juce::Colour (0xff2c6a59) : over ? juce::Colour (0xff4a4d58) : col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (on ? col::green : col::ink); g.setFont (labelFont (14.0f, true, 0.08f)); g.drawText (text.toUpperCase(), getLocalBounds(), juce::Justification::centred, false);
    }
    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
    juce::String text; std::function<void()> onClick; bool over = false, on = false;
};

struct PurrOneEditor::Check : public juce::Component
{
    Check (Bubbles& b) : bubbles (b) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics& g) override
    {
        const juce::Rectangle<float> box (1.0f, ((float) getHeight() - 14.0f) * 0.5f, 14.0f, 14.0f);
        g.setColour (bubbles.on ? col::blue : col::field); g.fillRoundedRectangle (box, 2.0f);
        g.setColour (bubbles.on ? col::blue : col::line); g.drawRoundedRectangle (box, 2.0f, 1.0f);
        if (bubbles.on) { juce::Path p; p.startNewSubPath (box.getX() + 3.0f, box.getCentreY()); p.lineTo (box.getX() + 6.0f, box.getBottom() - 4.0f); p.lineTo (box.getRight() - 3.0f, box.getY() + 3.5f);
                          g.setColour (col::black); g.strokePath (p, juce::PathStrokeType (2.0f)); }
        g.setColour (col::inkDim); g.setFont (labelFont (14.0f, false, 0.08f)); g.drawText ("INFO BUBBLES", 22, 0, getWidth() - 22, getHeight(), juce::Justification::centredLeft, false);
    }
    void mouseDown (const juce::MouseEvent&) override { bubbles.on = ! bubbles.on; repaint(); }
    Bubbles& bubbles;
};

struct PurrOneEditor::NameBox : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
    NameBox() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setTooltip ("Click to open the preset browser. Double click to rename.\nDice: a random sound with small changes. Star: add or remove from your favourites."); }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::black); g.fillRoundedRectangle (r, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (col::ink); g.setFont (monoFont (15.0f)); g.drawText (name.toUpperCase(), 42, 0, getWidth() - 160, getHeight(), juce::Justification::centredLeft, true);
        g.setColour (col::green); g.setFont (monoFont (12.0f)); g.drawText (category.toUpperCase(), getWidth() - 118, 0, 80, getHeight(), juce::Justification::centredRight, false);
        // dice (random sound) on the left, star (favourite) on the right
        { const auto d = juce::Rectangle<float> (11.0f, getHeight() * 0.5f - 8.0f, 16.0f, 16.0f); g.setColour (col::inkDim); g.drawRoundedRectangle (d, 3.0f, 1.4f);
          for (auto p : { juce::Point<float> (0.28f, 0.28f), juce::Point<float> (0.72f, 0.28f), juce::Point<float> (0.5f, 0.5f), juce::Point<float> (0.28f, 0.72f), juce::Point<float> (0.72f, 0.72f) })
              g.fillEllipse (d.getX() + p.x * d.getWidth() - 1.3f, d.getY() + p.y * d.getHeight() - 1.3f, 2.6f, 2.6f); }
        { juce::Path star; star.addStar (juce::Point<float> ((float) getWidth() - 20.0f, getHeight() * 0.5f), 5, 3.6f, 8.0f, -1.5708f);
          if (fav) { g.setColour (col::yellow); g.fillPath (star); } else { g.setColour (col::inkDim); g.strokePath (star, juce::PathStrokeType (1.3f)); } }
    }
    // a single click opens the list after a short wait, so a double click (rename) is not swallowed by the menu
    bool onDice (const juce::MouseEvent& e) const { return e.x < 38; }
    bool onStar (const juce::MouseEvent& e) const { return e.x > getWidth() - 38; }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition()) || e.getNumberOfClicks() != 1) return;
        if (onDice (e)) { if (onRandom) onRandom(); return; }
        if (onStar (e)) { if (onFav) onFav(); return; }
        startTimer (280);
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override { if (onDice (e) || onStar (e)) return; stopTimer(); if (onDouble) onDouble(); }
    void timerCallback() override { stopTimer(); if (onClick) onClick(); }
    juce::String name { "Init" }, category { "Init" }; bool fav = false; std::function<void()> onClick, onDouble, onRandom, onFav;
};

//==============================================================================
struct PurrOneEditor::Content : public juce::Component
{
    Content (PurrOneEditor& o) : owner (o), proc (o.proc),
        prev ("<"), next (">"), save ("Save"), rename ("Rename"), init ("Init"), zoom ("100%"), playBtn ("Play"), check (o.bubbles), meter (o.proc), keys (o.proc.keyState)
    {
        setOpaque (true);
        logo = juce::Drawable::createFromSVG (*juce::XmlDocument::parse (kLogoSvg));
        auto& a = proc.apvts;
        for (auto& d : L::knobs) { if (auto* p = a.getParameter (d.id)) { knobs.add (new Knob (*p, d, o.bubbles)); addAndMakeVisible (knobs.getLast()); } }
        for (auto& d : L::togs)  { if (auto* p = a.getParameter (d.id)) { togs.add (new LedToggle (*p, d, o.bubbles)); addAndMakeVisible (togs.getLast()); } }
        for (auto& d : L::segs)  { if (auto* p = a.getParameter (d.id)) { segs.add (new Segments (*p, d, o.bubbles)); addAndMakeVisible (segs.getLast()); } }
        const auto& m = L::misc;
        displays.add (new Display (a, Display::filter, m.filt)); displays.add (new Display (a, Display::fenv, m.fenv));
        displays.add (new Display (a, Display::aenv, m.aenv));   displays.add (new Display (a, Display::eq, m.eq));
        for (auto* d : displays) addAndMakeVisible (d);
        addAndMakeVisible (meter); addAndMakeVisible (keys);
        {   // pitch wheel and mod wheel left of the keyboard
            const auto kr = rc (m.keys); keys.setBounds (kr.withTrimmedLeft (84));
            pitchWheel = std::make_unique<Wheel> (true, [this] { return proc.bendShown.load(); }, [this] (float v) { proc.uiBend.store (v); proc.uiBendNew.store (true); }, nullptr, col::pink);
            pitchWheel->setBounds (kr.getX(), kr.getY(), 36, kr.getHeight());
            pitchWheel->setTooltip ("Pitch wheel. Springs back to the middle. The range (2, 5, 12 or 24 semitones, up and down) is on the Settings page. A MIDI keyboard moves it too.");
            addAndMakeVisible (*pitchWheel);
            if (auto* wp = proc.apvts.getParameter (purrone::ids::wheel))
            {
                modWheel = std::make_unique<Wheel> (false, [wp] { return wp->getValue(); }, [wp] (float v) { wp->setValueNotifyingHost (v); }, [wp] (bool down) { if (down) wp->beginChangeGesture(); else wp->endChangeGesture(); }, col::green);
                modWheel->setBounds (kr.getX() + 42, kr.getY(), 36, kr.getHeight());
                modWheel->setTooltip ("Mod wheel. Same as the Wheel knob: anything routed to Wheel scales with it. Double-click to zero.");
                addAndMakeVisible (*modWheel);
            }
        }
        for (juce::Component* c : std::initializer_list<juce::Component*> { &prev, &next, &save, &rename, &init, &zoom, &playBtn, &check, &nameBox }) addAndMakeVisible (c);
        // header: the preset name box is narrower than the Sound-only mockup to make room for the page tabs
        prev.setBounds (rc (m.prev)); nameBox.setBounds (298, 13, 380, 32); next.setBounds (684, 14, 33, 31); save.setBounds (723, 14, 63, 31); rename.setBounds (791, 14, 87, 31); init.setBounds (883, 14, 53, 31);
        check.setBounds (rc (m.bubbles).withWidth (140)); zoom.setBounds (rc (m.zoom));
        o.bubbles.learnMenu = [this] (const juce::String& id)
        {
            juce::PopupMenu m; const int cc = proc.ccFor (id); const bool learning = proc.learningId() == id;
            m.addItem (1, learning ? "Cancel MIDI learn" : "MIDI learn: then move a control on your controller");
            if (cc >= 0) m.addItem (2, "Forget MIDI CC " + juce::String (cc));
            m.addItem (3, "Reset to default");
            m.showMenuAsync (juce::PopupMenu::Options(), [this, id] (int r)
            {
                if (r == 1) { if (proc.learningId() == id) proc.cancelLearn(); else proc.startLearn (id); }
                else if (r == 2) proc.forgetCC (id);
                else if (r == 3) if (auto* p = proc.apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
                repaint();
            });
        };
        playBtn.setBounds (1522, 14, 100, 31); playBtn.setTooltip ("Starts and stops the sequence or arp from any page. In a DAW the transport, a key or a start note can do it too (Seq / Arp page).");
        playBtn.onClick = [this] { const bool on = ! proc.playButton.load(); if (on && (int) std::lround (proc.apvts.getRawParameterValue (purrone::ids::seqMode)->load()) == 0) proc.setParam (purrone::ids::seqMode, 1.0f); proc.playButton.store (on); };
        { static const char* names[4] = { "Sound", "Seq / Arp", "FX", "Settings" }; static const int xs[4] = { 945, 1019, 1119, 1175 }, ws[4] = { 70, 96, 52, 88 };
          for (int i = 0; i < 4; ++i) { auto* t = new Button (names[i], i == 0 ? "The synth" : i == 1 ? "Step sequencer and arpeggiator" : i == 2 ? "Effects, EQ and analyzer" : "Banks, tuning, quality and window"); t->setBounds (xs[i], 14, ws[i], 31); t->onClick = [this, i] { owner.showView (i); }; tabs.add (t); addAndMakeVisible (t); } }
        seqPage = std::make_unique<purrone::ui::SeqPage> (proc, o.bubbles); addChildComponent (*seqPage);
        fxHost = std::make_unique<purrone::ui::FxHost> (proc, o.bubbles); addChildComponent (*fxHost);
        settingsPage = std::make_unique<purrone::ui::SettingsPage> (proc, o.bubbles, [&o] { return o.zoomPercent(); }, [&o] (int v) { static const int pc[5] = { 70, 85, 100, 115, 130 }; o.setZoomPercent (pc[juce::jlimit (0, 4, v)]); });
        addChildComponent (*settingsPage);
        keys.toFront (false);
        save.setTooltip ("Stores the current sound as your own. Asks before it overwrites one."); rename.setTooltip ("Rename this sound. A factory sound is saved as your own copy under the new name.");
        editor.setFont (monoFont (15.0f)); editor.setMultiLine (false); editor.setBorder ({ 0, 10, 0, 10 }); editor.setJustification (juce::Justification::centredLeft);
        editor.setBounds (298, 13, 380, 32); editor.setVisible (false); editor.setInputRestrictions (60); addChildComponent (editor);
    }

    void setView (int v)
    {
        view = v; const bool snd = v == 0;
        for (auto* k : knobs) k->setVisible (snd); for (auto* t : togs) t->setVisible (snd); for (auto* sg : segs) sg->setVisible (snd); for (auto* d : displays) d->setVisible (snd);
        meter.setVisible (snd); seqPage->setVisible (v == 1); fxHost->setVisible (v == 2); settingsPage->setVisible (v == 3);
        for (int i = 0; i < tabs.size(); ++i) { tabs[i]->on = i == v; tabs[i]->repaint(); }
        repaint();
    }
    void tickHeader() { const juce::String li = proc.learningId(); if (li != learnShown) { learnShown = li; repaint(); }  const bool on = proc.playButton.load(); const juce::String t = on ? "Stop" : "Play"; if (playBtn.text != t || playBtn.on != on) { playBtn.text = t; playBtn.on = on; playBtn.repaint(); } }
    void tickPages() { tickHeader(); if (view == 1) seqPage->tick(); else if (view == 2) fxHost->tick(); else if (view == 3) settingsPage->tick(); }

    // double click on either wood cheek: back to the default window size (100%)
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto& m = L::misc;
        if (rc (m.cheekL).contains (e.getPosition()) || rc (m.cheekR).contains (e.getPosition())) owner.resetZoom();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& m = L::misc;
        g.fillAll (col::panel);
        { // panel background
            const auto pr = rc (m.panel).toFloat();
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1e1f23), pr.getCentreX(), pr.getY(), col::panel, pr.getCentreX(), pr.getY() + 500.0f, true)); g.fillRect (pr);
        }
        paintWood (g, rc (m.cheekL), true); paintWood (g, rc (m.cheekR), false);

        if (logo != nullptr) logo->drawWithin (g, juce::Rectangle<float> ((float) m.brand.x, (float) m.brand.y, 38.0f, 38.0f), juce::RectanglePlacement::centred, 1.0f);
        g.setColour (col::ink); g.setFont (labelFont (34.0f, true, 0.12f)); g.drawText ("PurrOne", m.brand.x + 48, m.brand.y - 2, 160, 42, juce::Justification::centredLeft, false);
        g.setColour (col::line); g.drawHorizontalLine (m.header.y + m.header.h - 1, (float) m.header.x, (float) (m.header.x + m.header.w));

        if (view == 0) for (auto& s : L::secs)
        {
            const auto r = rc (s.r).toFloat();
            g.setColour (col::panel2); g.fillRoundedRectangle (r, 3.0f);
            g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            g.setColour (juce::Colour (0xff3a3d48)); g.fillRect (r.getX() + 1.0f, r.getY(), r.getWidth() - 2.0f, 3.0f);
            g.setColour (col::ink); g.setFont (labelFont (16.0f, true, 0.2f)); g.drawText (s.title, (int) r.getX() + 10, (int) r.getY() + 8, 200, 18, juce::Justification::centredLeft, false);
            g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.1f)); g.drawText (s.sub, (int) r.getX() + 90, (int) r.getY() + 8, (int) r.getWidth() - 100, 18, juce::Justification::centredRight, false);
        }
        if (view == 0) { g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.12f)); g.drawText ("UNISON", 713, 543, 110, 14, juce::Justification::centredLeft, false); }
        if (view == 0) { g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.12f)); g.drawText ("DESTINATIONS", m.keys.x + 0, 344, 110, 14, juce::Justification::centredLeft, false); }

        // keyboard panel and status line
        const juce::Rectangle<float> kb (50.0f, 685.0f, 1600.0f, 129.0f);
        g.setColour (col::panel2); g.fillRoundedRectangle (kb, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (kb.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (juce::Colour (0xff3a3d48)); g.fillRect (kb.getX() + 1.0f, kb.getY(), kb.getWidth() - 2.0f, 3.0f);
        juce::AttributedString as; as.setJustification (juce::Justification::centredLeft);
        if (learnShown.isNotEmpty())
        {
            as.append ("MIDI learn: ", labelFont (14.0f, true, 0.03f), col::pink);
            as.append ("move a knob or fader on your controller to map it to " + learnShown + ". Right-click the control again to cancel.", labelFont (14.0f, false, 0.03f), col::ink);
        }
        else
        {
        as.append ("Play with the mouse or keys ", labelFont (14.0f, false, 0.03f), col::inkDim);
        as.append ("A W S E D F T G Y H U J K", labelFont (14.0f, true, 0.03f), col::ink);
        as.append (", Z and X shift the octave. Right-click a knob for MIDI learn", labelFont (14.0f, false, 0.03f), col::inkDim);
        }
        as.draw (g, juce::Rectangle<float> ((float) m.status.x, (float) m.status.y - 1.0f, 900.0f, 18.0f));
    }

    static void paintWood (juce::Graphics& g, juce::Rectangle<int> r, bool left)
    {
        const juce::Colour c[4] = { juce::Colour (0xff5B3A22), juce::Colour (0xff6E4729), juce::Colour (0xff4E3120), juce::Colour (0xff77502F) };
        const int w[4] = { 3, 4, 2, 5 }; int x = r.getX(), i = left ? 0 : 1;
        while (x < r.getRight()) { g.setColour (c[i & 3]); g.fillRect (x, r.getY(), juce::jmin (w[i & 3], r.getRight() - x), r.getHeight()); x += w[i & 3]; ++i; }
        const float x0 = left ? (float) r.getRight() - 8.0f : (float) r.getX(), x1 = left ? (float) r.getRight() : (float) r.getX() + 8.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (left ? 0.0f : 0.6f), x0, 0.0f, juce::Colours::black.withAlpha (left ? 0.6f : 0.0f), x1, 0.0f, false));
        g.fillRect (juce::Rectangle<float> (juce::jmin (x0, x1), (float) r.getY(), 8.0f, (float) r.getHeight()));
    }

    PurrOneEditor& owner; PurrOneProcessor& proc;
    std::unique_ptr<juce::Drawable> logo;
    juce::OwnedArray<Knob> knobs; juce::OwnedArray<LedToggle> togs; juce::OwnedArray<Segments> segs; juce::OwnedArray<Display> displays;
    int view = 0; juce::OwnedArray<Button> tabs; std::unique_ptr<purrone::ui::SeqPage> seqPage; std::unique_ptr<purrone::ui::FxHost> fxHost; std::unique_ptr<purrone::ui::SettingsPage> settingsPage;
    juce::String learnShown; Button prev, next, save, rename, init, zoom, playBtn; Check check; NameBox nameBox; juce::TextEditor editor; Meter meter; Keys keys; std::unique_ptr<Wheel> pitchWheel, modWheel;
};

//==============================================================================
// The preset browser: a popup over the window with bank chips, a search box, type chips, a history and the list of sounds.
// Same layout, order and wording as the browser in docs/mockup/purrone-mockup.html (a 1040 x 600 panel in the 1700 x 828 window).
struct PurrOneEditor::Browser : public juce::Component, private juce::KeyListener
{
    struct Entry { juce::String name, cat; int bank = 0, n = 0, factoryIndex = -1; };

    struct Chip : public juce::Component
    {
        Chip (const juce::String& t, bool star = false) : text (t), hasStar (star) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        int widthFor() const { juce::GlyphArrangement ga; ga.addLineOfText (labelFont (13.0f, true, 0.08f), text.toUpperCase(), 0.0f, 0.0f); return juce::roundToInt (ga.getBoundingBox (0, -1, true).getWidth()) + 24 + (hasStar ? 16 : 0); }
        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (on ? col::green : over ? juce::Colour (0xff23252c) : col::panel2); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (on ? col::green : col::line); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
            const auto ink = on ? juce::Colour (0xff06130f) : col::ink; g.setColour (ink);
            auto tr = getLocalBounds();
            if (hasStar) { juce::Path s; s.addStar (juce::Point<float> (17.0f, (float) getHeight() * 0.5f), 5, 3.2f, 7.0f, -1.5708f); g.fillPath (s); tr.removeFromLeft (16); }
            g.setFont (labelFont (13.0f, true, 0.08f)); g.drawText (text.toUpperCase(), tr, juce::Justification::centred, false);
        }
        void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override { over = false; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
        juce::String text; bool hasStar, on = false, over = false; std::function<void()> onClick;
    };

    static constexpr int rowH = 38, histH = 28;

    // the list of sounds (lives inside a Viewport)
    struct Rows : public juce::Component
    {
        explicit Rows (Browser& b) : owner (b) { setWantsKeyboardFocus (false); }
        void paint (juce::Graphics& g) override
        {
            const auto clip = g.getClipBounds(); const int n = (int) owner.vis.size();
            const int first = juce::jmax (0, clip.getY() / rowH), last = juce::jmin (n - 1, clip.getBottom() / rowH);
            const auto favs = owner.proc.favourites();
            for (int i = first; i <= last; ++i)
            {
                const auto& e = owner.entries[(size_t) owner.vis[(size_t) i]]; const juce::Rectangle<int> r (0, i * rowH, getWidth(), rowH);
                if (i == owner.curRow) { g.setColour (juce::Colour (0xff2c2f38)); g.fillRect (r); }
                else if (i == hover)   { g.setColour (col::panel2); g.fillRect (r); }
                g.setColour (juce::Colour (0xff14151a)); g.fillRect (juce::Rectangle<int> (r.getX(), r.getBottom() - 1, r.getWidth(), 1));
                g.setColour (col::inkDim); g.setFont (monoFont (13.0f));
                g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + e.bank)) + juce::String (e.n).paddedLeft ('0', 3), r.getX() + 12, r.getY(), 56, rowH, juce::Justification::centredLeft, false);
                g.setColour (col::ink); g.drawText (e.name, r.getX() + 66, r.getY(), getWidth() - 66 - 82, rowH, juce::Justification::centredLeft, true);
                g.setColour (col::green); g.setFont (monoFont (10.0f)); g.drawText (e.cat.toUpperCase(), getWidth() - 88, r.getY(), 46, rowH, juce::Justification::centredRight, false);
                juce::Path s; s.addStar (juce::Point<float> ((float) getWidth() - 20.0f, (float) r.getCentreY()), 5, 3.0f, 6.8f, -1.5708f);
                if (favs.contains (e.name)) { g.setColour (col::yellow); g.fillPath (s); } else { g.setColour (col::inkDim); g.strokePath (s, juce::PathStrokeType (1.2f)); }
            }
        }
        int rowAt (const juce::MouseEvent& e) const { const int r = e.y / rowH; return e.y >= 0 && r < (int) owner.vis.size() ? r : -1; }
        void mouseMove (const juce::MouseEvent& e) override { const int r = rowAt (e); if (r != hover) { hover = r; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            const int r = rowAt (e); if (r < 0 || e.mouseWasDraggedSinceMouseDown()) return;
            if (e.x > getWidth() - 44) owner.toggleFav (r); else owner.loadRow (r);
            owner.search.grabKeyboardFocus();
        }
        void mouseDoubleClick (const juce::MouseEvent& e) override { const int r = rowAt (e); if (r >= 0 && e.x <= getWidth() - 44) { owner.loadRow (r); owner.closeSoon(); } }
        Browser& owner; int hover = -1;
    };

    // the history of sounds loaded this session
    struct Hist : public juce::Component
    {
        explicit Hist (Browser& b) : owner (b) { setWantsKeyboardFocus (false); }
        void paint (juce::Graphics& g) override
        {
            const auto& list = owner.proc.recentSounds;
            if (list.isEmpty()) { g.setColour (col::inkDim); g.setFont (labelFont (14.0f)); g.drawText ("Sounds you load show up here.", 10, 8, getWidth() - 20, 20, juce::Justification::centredLeft, false); return; }
            for (int i = 0; i < list.size(); ++i)
            {
                const juce::Rectangle<int> r (0, i * histH, getWidth(), histH);
                if (i == hover) { g.setColour (col::panel2); g.fillRect (r); }
                g.setColour (col::ink); g.setFont (monoFont (12.0f)); g.drawText (list[i], r.getX() + 10, r.getY(), getWidth() - 16, histH, juce::Justification::centredLeft, true);
            }
        }
        int rowAt (const juce::MouseEvent& e) const { const int r = e.y / histH; return e.y >= 0 && r < owner.proc.recentSounds.size() ? r : -1; }
        void mouseMove (const juce::MouseEvent& e) override { const int r = rowAt (e); if (r != hover) { hover = r; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override { const int r = rowAt (e); if (r >= 0) { owner.loadByName (owner.proc.recentSounds[r]); owner.search.grabKeyboardFocus(); } }
        Browser& owner; int hover = -1;
    };

    explicit Browser (PurrOneEditor& o) : owner (o), proc (o.proc), closeBtn ("Close"), rows (*this), hist (*this)
    {
        setWantsKeyboardFocus (true);
        search.setFont (monoFont (14.0f)); search.setMultiLine (false); search.setBorder ({ 0, 10, 0, 10 }); search.setJustification (juce::Justification::centredLeft);
        search.setTextToShowWhenEmpty ("name or type", col::inkDim); search.setInputRestrictions (40);
        search.setSelectAllWhenFocused (true); search.addKeyListener (this);
        search.onTextChange = [this] { query = search.getText(); applyFilter(); };
        addAndMakeVisible (search);
        closeBtn.onClick = [this] { closeSoon(); }; addAndMakeVisible (closeBtn);
        listView.setViewedComponent (&rows, false); listView.setScrollBarsShown (true, false); listView.setScrollBarThickness (10); addAndMakeVisible (listView);
        histView.setViewedComponent (&hist, false); histView.setScrollBarsShown (true, false); histView.setScrollBarThickness (10); addAndMakeVisible (histView);
        buildEntries();
        for (auto b : { "ALL", "A", "B", "C", "D", "E", "F", "G", "H", "FAV" })
        {
            auto* c = bankChips.add (new Chip (juce::String (b) == "FAV" ? "Favourites" : b, juce::String (b) == "FAV")); const juce::String key (b);
            c->onClick = [this, key] { bankSel = key; applyFilter(); }; addAndMakeVisible (c);
        }
        for (auto& t : cats) { auto* c = typeChips.add (new Chip (t)); const juce::String key (t); c->onClick = [this, key] { typeSel = typeSel == key ? juce::String() : key; applyFilter(); }; addAndMakeVisible (c); }
        applyFilter();
    }

    void buildEntries()
    {
        entries.clear(); cats.clear(); const auto& fp = purrone::factoryPresets();
        for (size_t i = 0; i < fp.size(); ++i) { Entry e; e.name = fp[i].name; e.cat = fp[i].category; e.bank = (int) i / purrone::kBankSize; e.n = (int) i % purrone::kBankSize + 1; e.factoryIndex = (int) i; entries.push_back (e); }
        for (int b = purrone::kFactoryBanks; b < purrone::kBankCount; ++b) { int n = 1; for (auto& nm : proc.listBank (b)) { Entry e; e.name = nm; e.cat = "User"; e.bank = b; e.n = n++; entries.push_back (e); } }
        for (auto& e : entries) if (! cats.contains (e.cat)) cats.add (e.cat);
    }

    void applyFilter()
    {
        const auto keep = listView.getViewPositionY();
        const auto toks = juce::StringArray::fromTokens (query.toLowerCase(), " ", ""); const auto favs = proc.favourites();
        vis.clear();
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto& e = entries[i]; const juce::String bl = juce::String::charToString ((juce::juce_wchar) ('A' + e.bank));
            if (bankSel == "FAV") { if (! favs.contains (e.name)) continue; } else if (bankSel != "ALL" && bl != bankSel) continue;
            if (typeSel.isNotEmpty() && e.cat != typeSel) continue;
            const auto hay = (e.name + " " + e.cat + " " + bl).toLowerCase(); bool ok = true;
            for (auto& t : toks) if (t.isNotEmpty() && ! hay.contains (t)) { ok = false; break; }
            if (ok) vis.push_back ((int) i);
        }
        for (auto* c : bankChips) c->on = (c->hasStar ? juce::String ("FAV") : c->text) == bankSel;
        for (auto* c : typeChips) c->on = c->text == typeSel;
        for (auto* c : bankChips) c->repaint();
        for (auto* c : typeChips) c->repaint();
        syncCurrent();
        rows.setSize (listView.getMaximumVisibleWidth(), juce::jmax (1, (int) vis.size()) * rowH);
        listView.setViewPosition (0, keep); rows.repaint(); refreshHist(); repaint();
    }

    void syncCurrent() { curRow = -1; for (size_t i = 0; i < vis.size(); ++i) if (entries[(size_t) vis[i]].name == proc.currentPresetName) { curRow = (int) i; break; } }
    void refreshHist() { hist.setSize (histView.getMaximumVisibleWidth(), juce::jmax (histH, proc.recentSounds.size() * histH)); hist.repaint(); }
    void scrollToCurrent() { if (curRow >= 0 && listView.getHeight() > 0) listView.setViewPosition (0, juce::jmax (0, curRow * rowH - listView.getHeight() / 2 + rowH / 2)); }

    void loadEntry (const Entry& e)
    {
        if (e.factoryIndex >= 0) proc.setCurrentProgram (e.factoryIndex); else proc.loadUserPreset (e.name);
        owner.refreshNames(); syncCurrent(); rows.repaint(); refreshHist(); repaint();
    }
    void loadRow (int row) { if (row >= 0 && row < (int) vis.size()) loadEntry (entries[(size_t) vis[(size_t) row]]); }
    void loadByName (const juce::String& n) { for (auto& e : entries) if (e.name == n) { loadEntry (e); return; } }
    void toggleFav (int row) { proc.toggleFavourite (entries[(size_t) vis[(size_t) row]].name); owner.refreshNames(); applyFilter(); }
    void stepRow (int d)
    {
        const int n = (int) vis.size(); if (n == 0) return;
        const int next = curRow < 0 ? (d > 0 ? 0 : n - 1) : (curRow + d + n) % n;
        loadRow (next);
        const int y = next * rowH, top = listView.getViewPositionY();
        if (y < top) listView.setViewPosition (0, y); else if (y + rowH > top + listView.getHeight()) listView.setViewPosition (0, y + rowH - listView.getHeight());
    }
    void closeSoon() { juce::Component::SafePointer<PurrOneEditor> ed (&owner); juce::MessageManager::callAsync ([ed] { if (ed != nullptr) ed->closeBrowser(); }); }

    bool handleKey (const juce::KeyPress& k)
    {
        if (k == juce::KeyPress::escapeKey || k == juce::KeyPress::returnKey) { closeSoon(); return true; }
        if (k == juce::KeyPress::downKey) { stepRow (1); return true; }
        if (k == juce::KeyPress::upKey)   { stepRow (-1); return true; }
        return false;
    }
    bool keyPressed (const juce::KeyPress& k) override { return handleKey (k); }
    bool keyPressed (const juce::KeyPress& k, juce::Component*) override { return handleKey (k); }

    juce::Rectangle<int> panel() const { return juce::Rectangle<int> (1040, 600).withCentre (getLocalBounds().getCentre()); }
    static int flowWrap (juce::Rectangle<int> area, juce::OwnedArray<Chip>& chips)   // chips in rows that wrap; returns the y below the last row
    {
        int x = area.getX(), y = area.getY();
        for (auto* c : chips) { const int w = c->widthFor(); if (x + w > area.getRight() && x > area.getX()) { x = area.getX(); y += 33; } c->setBounds (x, y, w, 28); x += w + 5; }
        return y + 28;
    }
    void resized() override
    {
        auto p = panel().reduced (16, 14);
        titleRect = p.removeFromTop (40); p.removeFromTop (12);
        auto foot = p.removeFromBottom (32); footRect = foot; closeBtn.setBounds (foot.removeFromRight (90)); p.removeFromBottom (12);
        auto left = p.removeFromLeft (320); p.removeFromLeft (12);
        const int by = flowWrap (left, bankChips); listRect = left.withTrimmedTop (by - left.getY() + 8); listView.setBounds (listRect.reduced (1));
        auto r = p; labSearch = r.removeFromTop (18); search.setBounds (r.removeFromTop (34)); r.removeFromTop (6);
        labType = r.removeFromTop (18); const int ty = flowWrap (r, typeChips); r.removeFromTop (ty - r.getY() + 8);
        labHist = r.removeFromTop (18); noteRect = r.removeFromBottom (22); histRect = r.withTrimmedBottom (6); histView.setBounds (histRect.reduced (1));
        applyFilter();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.35f));
        const auto pr = panel().toFloat();
        g.setColour (juce::Colour (0xff0f1013)); g.fillRoundedRectangle (pr, 6.0f);
        g.setColour (col::line); g.drawRoundedRectangle (pr.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (col::ink); g.setFont (labelFont (23.0f, true, 0.22f)); g.drawText ("PRESET BROWSER", titleRect, juce::Justification::centredLeft, false);
        g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.16f));
        g.drawText ("SEARCH", labSearch, juce::Justification::centredLeft, false);
        g.drawText ("TYPE", labType, juce::Justification::centredLeft, false);
        g.drawText ("HISTORY (click to go back)", labHist, juce::Justification::centredLeft, false);
        for (auto r : { listRect, histRect }) { g.setColour (juce::Colour (0xff0a0b0e)); g.fillRect (r); g.setColour (col::line); g.drawRect (r, 1); }
        g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.04f));
        g.drawText ("Banks A to D are the factory sounds. E to H are yours.", noteRect, juce::Justification::centredLeft, false);
        g.drawText (juce::String ((int) vis.size()) + " of " + juce::String ((int) entries.size()) + " sounds", footRect.getX(), footRect.getY(), 200, footRect.getHeight(), juce::Justification::centredLeft, false);
        g.drawText ("Click loads. Double click loads and closes. Up and Down step. Esc closes.", footRect.getX() + 200, footRect.getY(), footRect.getWidth() - 320, footRect.getHeight(), juce::Justification::centred, false);
        if (vis.empty()) { g.setColour (col::inkDim); g.setFont (labelFont (14.0f)); g.drawFittedText (bankSel == "FAV" ? "No favourites yet. Click the star on a sound." : "Nothing matches. Try fewer words or another bank.", listRect.reduced (12, 10).removeFromTop (40), juce::Justification::topLeft, 2); }
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! panel().contains (e.getPosition())) closeSoon(); }

    PurrOneEditor& owner; PurrOneProcessor& proc;
    std::vector<Entry> entries; std::vector<int> vis; juce::StringArray cats;
    juce::String bankSel { "ALL" }, typeSel, query; int curRow = -1;
    juce::TextEditor search; Button closeBtn; Rows rows; Hist hist; juce::Viewport listView, histView;
    juce::OwnedArray<Chip> bankChips, typeChips; juce::Rectangle<int> listRect, histRect, noteRect, footRect, titleRect, labSearch, labType, labHist;
};

//==============================================================================
PurrOneEditor::PurrOneEditor (PurrOneProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    look = std::make_unique<Look>(); setLookAndFeel (look.get());
    content = std::make_unique<Content> (*this); addAndMakeVisible (*content); content->setSize (L::W, L::H);
    tooltip = std::make_unique<juce::TooltipWindow> (this, 450);

    content->prev.onClick = [this] { step (-1); }; content->next.onClick = [this] { step (1); };
    content->init.onClick = [this] { proc.setCurrentProgram (0); refreshNames(); };
    content->save.onClick = [this] { saveDialog(); };
    content->rename.onClick = [this] { beginRename(); };
    content->nameBox.onRandom = [this] { proc.randomSound(); refreshNames(); };
    content->nameBox.onFav = [this] { proc.toggleFavourite (proc.currentPresetName); refreshNames(); };
    content->nameBox.onClick = [this] { openBrowser(); }; content->nameBox.onDouble = [this] { beginRename(); };
    content->zoom.onClick = [this] { cycleZoom(); };
    content->editor.onReturnKey = [this] { commitRename(); };
    content->editor.onEscapeKey = [this] { content->editor.setVisible (false); };
    content->editor.onFocusLost = [this] { content->editor.setVisible (false); };
    refreshNames();
    content->setView (0);
    if (const char* v = std::getenv ("PURRONE_VIEW")) content->setView (juce::jlimit (0, 3, std::atoi (v)));   // test hook for screenshots
    if (const char* v = std::getenv ("PURRONE_FX")) content->fxHost->select (juce::jlimit (0, 4, std::atoi (v)));
    if (const char* v = std::getenv ("PURRONE_BAND")) content->fxHost->testBand (std::atoi (v));
    if (const char* v = std::getenv ("PURRONE_PARAMS"))   // test hook: "id=value,id=value" in real units
        for (auto& kv : juce::StringArray::fromTokens (juce::String (v), ",", ""))
            if (auto* prm = proc.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false).trim()))
                prm->setValueNotifyingHost (prm->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
    if (const char* v = std::getenv ("PURRONE_NOTE")) proc.keyState.noteOn (1, juce::jlimit (0, 127, std::atoi (v)), 0.8f);

    constrainer.setFixedAspectRatio ((double) L::W / L::H); constrainer.setSizeLimits (680, (int) (680 * (double) L::H / L::W), 2400, (int) (2400 * (double) L::H / L::W));
    setConstrainer (&constrainer);
    border = std::make_unique<juce::ResizableBorderComponent> (this, &constrainer); border->setBorderThickness (juce::BorderSize<int> (12)); addAndMakeVisible (*border);   // every side and corner
    corner = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer); addAndMakeVisible (*corner);
    setResizable (true, false);
    const int w = juce::jlimit (680, 2400, proc.editorWidth);
    setSize (w, (int) std::lround (w * (double) L::H / L::W));
    if (const char* v = std::getenv ("PURRONE_BROWSER"))   // test hook for screenshots: "BANK:search words" (both optional)
    {
        openBrowser(); const juce::String a (v);
        if (a.containsChar (':')) { browser->bankSel = a.upToFirstOccurrenceOf (":", false, false); browser->search.setText (a.fromFirstOccurrenceOf (":", false, false), true); browser->applyFilter(); }
    }
    startTimerHz (30);
}

PurrOneEditor::~PurrOneEditor() { stopTimer(); setLookAndFeel (nullptr); }

void PurrOneEditor::resized()
{
    const float s = (float) getWidth() / (float) L::W;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, L::W, L::H);
    if (border) { border->setBounds (getLocalBounds()); border->toFront (false); }
    if (corner) corner->toFront (false);
    if (corner) corner->setBounds (getWidth() - 26, getHeight() - 26, 26, 26);
    proc.editorWidth = getWidth();
    content->zoom.text = juce::String (juce::roundToInt ((float) getWidth() / (float) kDefaultW * 100.0f)) + "%"; content->zoom.repaint();
}

void PurrOneEditor::cycleZoom()
{
    const int steps[] = { 70, 85, 100, 115, 130 }; const int cur = juce::roundToInt ((float) getWidth() / kDefaultW * 100.0f);
    int next = steps[0]; for (int v : steps) if (v > cur + 1) { next = v; break; }
    const int w = kDefaultW * next / 100; setSize (w, (int) std::lround (w * (double) L::H / L::W));
}

void PurrOneEditor::setZoomPercent (int pct) { const int w = kDefaultW * pct / 100; setSize (w, (int) std::lround (w * (double) L::H / L::W)); }
int PurrOneEditor::zoomPercent() const
{
    static const int pc[5] = { 70, 85, 100, 115, 130 }; const int cur = juce::roundToInt ((float) getWidth() / kDefaultW * 100.0f);
    int best = 2, bd = 1000; for (int i = 0; i < 5; ++i) if (std::abs (pc[i] - cur) < bd) { bd = std::abs (pc[i] - cur); best = i; }
    return bd <= 3 ? best : -1;
}
void PurrOneEditor::showView (int v) { content->setView (v); }

void PurrOneEditor::resetZoom() { setSize (kDefaultW, (int) std::lround (kDefaultW * (double) L::H / L::W)); }

void PurrOneEditor::timerCallback()
{
    if (! isShowing()) return;
    for (auto* d : content->displays) d->tick();
    content->meter.tick();
    content->tickPages();
    auto& nb = content->nameBox;
    if (nb.name != proc.currentPresetName) refreshNames();
}

//==============================================================================
void PurrOneEditor::refreshNames()
{
    names.clear();
    for (auto& f : purrone::factoryPresets()) names.add (f.name);
    numFactory = names.size();
    names.addArray (proc.listUserPresets());
    auto& nb = content->nameBox; nb.name = proc.currentPresetName; nb.category = "User";
    for (auto& f : purrone::factoryPresets()) if (proc.currentPresetName == f.name) nb.category = f.category;
    nb.fav = proc.isFavourite (proc.currentPresetName);
    nb.repaint();
    if (proc.currentPresetName.isNotEmpty() && (proc.recentSounds.isEmpty() || proc.recentSounds[0] != proc.currentPresetName))
    {
        proc.recentSounds.removeString (proc.currentPresetName); proc.recentSounds.insert (0, proc.currentPresetName);
        while (proc.recentSounds.size() > 14) proc.recentSounds.remove (proc.recentSounds.size() - 1);
    }
    if (browser != nullptr) browser->repaint();
}

void PurrOneEditor::step (int dir)
{
    if (names.isEmpty()) return;
    int cur = names.indexOf (proc.currentPresetName); const int n = names.size();
    const int next = cur < 0 ? 0 : (cur + dir + n) % n;
    if (next < numFactory) proc.setCurrentProgram (next); else proc.loadUserPreset (names[next]);
    refreshNames();
}

void PurrOneEditor::openBrowser()
{
    if (browser != nullptr) return;
    refreshNames();
    browser = std::make_unique<Browser> (*this);
    content->addAndMakeVisible (*browser);
    browser->setBounds (content->getLocalBounds()); browser->toFront (true); browser->scrollToCurrent();
    browser->search.grabKeyboardFocus();
}

void PurrOneEditor::closeBrowser()
{
    if (browser == nullptr) return;
    content->removeChildComponent (browser.get());
    browser.reset(); refreshNames();
}

void PurrOneEditor::beginRename()
{
    auto& e = content->editor; e.setText (proc.currentPresetName, false); e.setVisible (true); e.toFront (true); e.grabKeyboardFocus(); e.selectAll();
}

void PurrOneEditor::commitRename()
{
    auto& e = content->editor; const auto nm = e.getText().trim(); const auto cur = proc.currentPresetName;
    e.setVisible (false);
    if (nm.isEmpty() || nm == cur) return;
    if (proc.listUserPresets().contains (cur)) { proc.renameUserPreset (cur, nm); refreshNames(); }
    else saveNamed (nm);                                       // factory sound: becomes your own copy under the new name
}

void PurrOneEditor::saveNamed (const juce::String& nm)
{
    if (proc.listUserPresets().contains (juce::File::createLegalFileName (nm.trim()).substring (0, 60)))   // never overwrite silently
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Overwrite sound?")
            .withMessage ("A sound called \"" + nm + "\" already exists. Overwrite it with the current settings?").withButton ("Overwrite").withButton ("Cancel"),
            [this, nm] (int pick) { if (pick == 1) { proc.saveUserPreset (nm); refreshNames(); } });
    }
    else { proc.saveUserPreset (nm); refreshNames(); }
}

void PurrOneEditor::saveDialog()
{
    dialog = std::make_unique<juce::AlertWindow> ("Save sound", "Name for this sound", juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor ("name", proc.currentPresetName == "Init" ? "My sound" : proc.currentPresetName);
    dialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey)); dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int r)
    {
        if (r == 1 && dialog != nullptr) saveNamed (dialog->getTextEditorContents ("name"));
        dialog.reset();
    }), false);
}
