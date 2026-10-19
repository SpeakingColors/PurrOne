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
        g.setColour (over ? juce::Colour (0xff23252c) : col::panel2); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (over ? juce::Colour (0xff4a4d58) : col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (col::ink); g.setFont (labelFont (14.0f, true, 0.08f)); g.drawText (text.toUpperCase(), getLocalBounds(), juce::Justification::centred, false);
    }
    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && onClick) onClick(); }
    juce::String text; std::function<void()> onClick; bool over = false;
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
    NameBox() { setMouseCursor (juce::MouseCursor::PointingHandCursor); setTooltip ("Click to pick a sound. Double click to rename."); }
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (col::black); g.fillRoundedRectangle (r, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (col::ink); g.setFont (monoFont (15.0f)); g.drawText (name.toUpperCase(), 12, 0, getWidth() - 100, getHeight(), juce::Justification::centredLeft, true);
        g.setColour (col::green); g.setFont (monoFont (12.0f)); g.drawText (category.toUpperCase(), getWidth() - 90, 0, 78, getHeight(), juce::Justification::centredRight, false);
    }
    // a single click opens the list after a short wait, so a double click (rename) is not swallowed by the menu
    void mouseUp (const juce::MouseEvent& e) override { if (getLocalBounds().contains (e.getPosition()) && e.getNumberOfClicks() == 1) startTimer (280); }
    void mouseDoubleClick (const juce::MouseEvent&) override { stopTimer(); if (onDouble) onDouble(); }
    void timerCallback() override { stopTimer(); if (onClick) onClick(); }
    juce::String name { "Init" }, category { "Init" }; std::function<void()> onClick, onDouble;
};

//==============================================================================
struct PurrOneEditor::Content : public juce::Component
{
    Content (PurrOneEditor& o) : owner (o), proc (o.proc),
        prev ("<"), next (">"), save ("Save"), rename ("Rename"), init ("Init"), zoom ("100%"), check (o.bubbles), meter (o.proc), keys (o.proc.keyState)
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
        keys.setBounds (rc (m.keys));
        for (juce::Component* c : std::initializer_list<juce::Component*> { &prev, &next, &save, &rename, &init, &zoom, &check, &nameBox }) addAndMakeVisible (c);
        prev.setBounds (rc (m.prev)); next.setBounds (rc (m.next)); save.setBounds (rc (m.save)); rename.setBounds (rc (m.rename)); init.setBounds (rc (m.init));
        nameBox.setBounds (rc (m.pname)); check.setBounds (rc (m.bubbles).withWidth (140)); zoom.setBounds (rc (m.zoom));
        save.setTooltip ("Stores the current sound as your own. Asks before it overwrites one."); rename.setTooltip ("Rename this sound. A factory sound is saved as your own copy under the new name.");
        editor.setFont (monoFont (15.0f)); editor.setMultiLine (false); editor.setBorder ({ 0, 10, 0, 10 }); editor.setJustification (juce::Justification::centredLeft);
        editor.setBounds (rc (m.pname)); editor.setVisible (false); editor.setInputRestrictions (60); addChildComponent (editor);
    }

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

        for (auto& s : L::secs)
        {
            const auto r = rc (s.r).toFloat();
            g.setColour (col::panel2); g.fillRoundedRectangle (r, 3.0f);
            g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
            g.setColour (juce::Colour (0xff3a3d48)); g.fillRect (r.getX() + 1.0f, r.getY(), r.getWidth() - 2.0f, 3.0f);
            g.setColour (col::ink); g.setFont (labelFont (16.0f, true, 0.2f)); g.drawText (s.title, (int) r.getX() + 10, (int) r.getY() + 8, 200, 18, juce::Justification::centredLeft, false);
            g.setColour (col::inkDim); g.setFont (labelFont (12.0f, false, 0.1f)); g.drawText (s.sub, (int) r.getX() + 90, (int) r.getY() + 8, (int) r.getWidth() - 100, 18, juce::Justification::centredRight, false);
        }
        g.setColour (col::inkDim); g.setFont (labelFont (13.0f, false, 0.12f)); g.drawText ("DESTINATIONS", m.keys.x + 0, 344, 110, 14, juce::Justification::centredLeft, false);

        // keyboard panel and status line
        const juce::Rectangle<float> kb (50.0f, 685.0f, 1600.0f, 129.0f);
        g.setColour (col::panel2); g.fillRoundedRectangle (kb, 3.0f); g.setColour (col::line); g.drawRoundedRectangle (kb.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (juce::Colour (0xff3a3d48)); g.fillRect (kb.getX() + 1.0f, kb.getY(), kb.getWidth() - 2.0f, 3.0f);
        juce::AttributedString as; as.setJustification (juce::Justification::centredLeft);
        as.append ("Play with the mouse or keys ", labelFont (14.0f, false, 0.03f), col::inkDim);
        as.append ("A W S E D F T G Y H U J K", labelFont (14.0f, true, 0.03f), col::ink);
        as.append (", Z and X shift the octave", labelFont (14.0f, false, 0.03f), col::inkDim);
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
    Button prev, next, save, rename, init, zoom; Check check; NameBox nameBox; juce::TextEditor editor; Meter meter; Keys keys;
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
    content->nameBox.onClick = [this] { openList(); }; content->nameBox.onDouble = [this] { beginRename(); };
    content->zoom.onClick = [this] { cycleZoom(); };
    content->editor.onReturnKey = [this] { commitRename(); };
    content->editor.onEscapeKey = [this] { content->editor.setVisible (false); };
    content->editor.onFocusLost = [this] { content->editor.setVisible (false); };
    refreshNames();

    constrainer.setFixedAspectRatio ((double) L::W / L::H); constrainer.setSizeLimits (680, (int) (680 * (double) L::H / L::W), 2400, (int) (2400 * (double) L::H / L::W));
    setConstrainer (&constrainer);
    border = std::make_unique<juce::ResizableBorderComponent> (this, &constrainer); border->setBorderThickness (juce::BorderSize<int> (6)); addAndMakeVisible (*border);   // every side and corner
    corner = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer); addAndMakeVisible (*corner);
    setResizable (true, false);
    const int w = juce::jlimit (680, 2400, proc.editorWidth);
    setSize (w, (int) std::lround (w * (double) L::H / L::W));
    startTimerHz (30);
}

PurrOneEditor::~PurrOneEditor() { stopTimer(); setLookAndFeel (nullptr); }

void PurrOneEditor::resized()
{
    const float s = (float) getWidth() / (float) L::W;
    content->setTransform (juce::AffineTransform::scale (s));
    content->setBounds (0, 0, L::W, L::H);
    if (border) border->setBounds (getLocalBounds());
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

void PurrOneEditor::resetZoom() { setSize (kDefaultW, (int) std::lround (kDefaultW * (double) L::H / L::W)); }

void PurrOneEditor::timerCallback()
{
    if (! isShowing()) return;
    for (auto* d : content->displays) d->tick();
    content->meter.tick();
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
    nb.repaint();
}

void PurrOneEditor::step (int dir)
{
    if (names.isEmpty()) return;
    int cur = names.indexOf (proc.currentPresetName); const int n = names.size();
    const int next = cur < 0 ? 0 : (cur + dir + n) % n;
    if (next < numFactory) proc.setCurrentProgram (next); else proc.loadUserPreset (names[next]);
    refreshNames();
}

void PurrOneEditor::openList()
{
    refreshNames();
    juce::PopupMenu menu; int id = 1;
    for (auto& f : purrone::factoryPresets()) { menu.addItem (id, juce::String (f.category).toUpperCase() + "   " + f.name, true, proc.currentPresetName == f.name); ++id; }
    if (names.size() > numFactory) { menu.addSeparator(); for (int i = numFactory; i < names.size(); ++i) menu.addItem (id++, "USER   " + names[i], true, proc.currentPresetName == names[i]); }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&content->nameBox).withMinimumWidth (content->nameBox.getWidth()).withStandardItemHeight (26),
                        [this] (int r) { if (r <= 0) return; const int i = r - 1; if (i < numFactory) proc.setCurrentProgram (i); else proc.loadUserPreset (names[i]); refreshNames(); });
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
