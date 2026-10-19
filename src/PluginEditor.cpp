#include "PluginEditor.h"
#include "Presets.h"

namespace { const int kCheek = 44, kMeterW = 110, kTop = 44; }

PurrOneEditor::PurrOneEditor (PurrOneProcessor& p) : AudioProcessorEditor (&p), proc (p), generic (p)
{
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn); addAndMakeVisible (saveBtn); addAndMakeVisible (renameBtn); addAndMakeVisible (generic);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        int id = presetBox.getSelectedId(); if (id <= 0) return;
        int nf = (int) purrone::factoryPresets().size();
        if (id <= nf) proc.setCurrentProgram (id - 1); else proc.loadUserPreset (userNames[id - nf - 1]);
    };
    prevBtn.onClick = [this] { step (-1); }; nextBtn.onClick = [this] { step (1); };
    saveBtn.onClick = [this]
    {
        saveDialog = std::make_unique<juce::AlertWindow> ("Save sound", "Name for this sound", juce::MessageBoxIconType::NoIcon);
        saveDialog->addTextEditor ("name", proc.currentPresetName == "Init" ? "My sound" : proc.currentPresetName);
        saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey)); saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int r)
        {
            if (r == 1 && saveDialog != nullptr)
            {
                const auto nm = saveDialog->getTextEditorContents ("name");
                if (proc.listUserPresets().contains (juce::File::createLegalFileName (nm.trim()).substring (0, 60)))   // never overwrite silently
                {
                    juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Overwrite sound?")
                        .withMessage ("A sound called \"" + nm + "\" already exists. Overwrite it with the current settings?").withButton ("Overwrite").withButton ("Cancel"),
                        [this, nm] (int pick) { if (pick == 1) { proc.saveUserPreset (nm); refreshPresetBox(); } });
                }
                else { proc.saveUserPreset (nm); refreshPresetBox(); }
            }
            saveDialog.reset();
        }), false);
    };
    renameBtn.onClick = [this]
    {
        const auto cur = proc.currentPresetName;
        if (! proc.listUserPresets().contains (cur)) { saveBtn.onClick(); return; }   // factory sound: save it as your own copy under a new name
        saveDialog = std::make_unique<juce::AlertWindow> ("Rename sound", "New name", juce::MessageBoxIconType::NoIcon);
        saveDialog->addTextEditor ("name", cur);
        saveDialog->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey)); saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, cur] (int r)
        {
            if (r == 1 && saveDialog != nullptr) { proc.renameUserPreset (cur, saveDialog->getTextEditorContents ("name")); refreshPresetBox(); }
            saveDialog.reset();
        }), false);
    };
    presetBox.addMouseListener (this, true);   // double click the name to rename
    setResizable (true, true); setResizeLimits (720, 480, 1600, 1200); setSize (980, 760);
    startTimerHz (30);
}
PurrOneEditor::~PurrOneEditor() { stopTimer(); }

void PurrOneEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    int id = 1;
    for (auto& f : purrone::factoryPresets()) presetBox.addItem (juce::String (f.category) + "  |  " + f.name, id++);
    userNames = proc.listUserPresets();
    if (userNames.size()) presetBox.addSeparator();
    for (auto& n : userNames) presetBox.addItem ("User  |  " + n, id++);
    presetBox.setText (proc.currentPresetName, juce::dontSendNotification);
}
void PurrOneEditor::step (int dir)
{
    int total = presetBox.getNumItems(), cur = presetBox.getSelectedId();
    if (total == 0) return; int next = cur <= 0 ? 1 : ((cur - 1 + dir + total) % total) + 1;
    presetBox.setSelectedId (next, juce::sendNotification);
}

void PurrOneEditor::timerCallback()
{
    if (! isShowing()) return;                                   // paint nothing while hidden
    auto up = [] (float& d, float v) { d = v > d ? v : d * 0.92f; };
    up (dispL, proc.meterL.load()); up (dispR, proc.meterR.load());
    holdL = std::max (dispL, holdL * 0.985f); holdR = std::max (dispR, holdR * 0.985f);
    if (proc.clipped.exchange (false)) clipFrames = 45; else if (clipFrames > 0) --clipFrames;
    repaint (meterArea);
}

static void wood (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d2210), (float) r.getX(), 0, juce::Colour (0xff3d2210), (float) r.getRight(), 0, false));
    juce::ColourGradient cg (juce::Colour (0xff3d2210), (float) r.getX(), 0, juce::Colour (0xff3d2210), (float) r.getRight(), 0, false);
    cg.addColour (0.4, juce::Colour (0xff7b4a26)); cg.addColour (0.75, juce::Colour (0xff5a3419)); g.setGradientFill (cg); g.fillRect (r);
    juce::Random rnd (7);
    for (int x = r.getX(); x < r.getRight(); x += 2)           // vertical grain lines, same seed every paint so it does not shimmer
    {
        g.setColour (juce::Colour::fromFloatRGBA (0.f, 0.f, 0.f, 0.05f + 0.20f * rnd.nextFloat()));
        float wob = rnd.nextFloat() * 2.0f; g.drawLine ((float) x + wob, (float) r.getY(), (float) x - wob, (float) r.getBottom(), 1.0f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawRect (r, 1);
}

void PurrOneEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15161a));
    wood (g, { 0, 0, kCheek, getHeight() }); wood (g, { getWidth() - kCheek, 0, kCheek, getHeight() });
    g.setColour (juce::Colours::white); g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("PurrOne", kCheek + 14, 6, 120, 32, juce::Justification::centredLeft);
    // output meter with dB scale, far right
    auto m = meterArea.reduced (8, 22); g.setColour (juce::Colours::white); g.setFont (12.0f); g.drawText ("OUT", meterArea.withHeight (20), juce::Justification::centred);
    const float dbTop = 0.0f, dbBot = -60.0f; auto yFor = [&] (float db) { return (float) m.getBottom() - juce::jlimit (0.0f, 1.0f, (db - dbBot) / (dbTop - dbBot)) * (float) m.getHeight(); };
    auto bar = [&] (juce::Rectangle<int> r, float lin, float hold)
    {
        g.setColour (juce::Colour (0xff0a0a0c)); g.fillRect (r);
        float db = lin > 0.00001f ? 20.0f * std::log10 (lin) : -90.0f; float y = yFor (db);
        juce::ColourGradient cg (juce::Colour (0xffff1383), 0, yFor (0.0f), juce::Colour (0xff02ebba), 0, yFor (-30.0f), false);
        cg.addColour (0.12, juce::Colour (0xffff9000)); cg.addColour (0.3, juce::Colour (0xffffd100)); g.setGradientFill (cg);
        g.fillRect (juce::Rectangle<float> ((float) r.getX(), y, (float) r.getWidth(), (float) r.getBottom() - y));
        float hd = hold > 0.00001f ? 20.0f * std::log10 (hold) : -90.0f; g.setColour (juce::Colours::white); g.fillRect ((float) r.getX(), yFor (hd), (float) r.getWidth(), 2.0f);
    };
    bar ({ m.getX(), m.getY(), 20, m.getHeight() }, dispL, holdL); bar ({ m.getX() + 24, m.getY(), 20, m.getHeight() }, dispR, holdR);
    g.setFont (juce::FontOptions (10.0f));
    for (int db : { 0, -6, -12, -18, -24, -36, -48, -60 }) { g.setColour (juce::Colours::lightgrey); g.drawText (juce::String (db), m.getX() + 48, (int) yFor ((float) db) - 6, 30, 12, juce::Justification::centredLeft); }
    g.setColour (clipFrames > 0 ? juce::Colour (0xffff1383) : juce::Colour (0xff2a2c33)); g.fillEllipse ((float) meterArea.getX() + 12, (float) meterArea.getBottom() - 16, 9.0f, 9.0f);
    g.setColour (juce::Colours::lightgrey); g.drawText ("clip", meterArea.getX() + 26, meterArea.getBottom() - 19, 40, 14, juce::Justification::centredLeft);
}

void PurrOneEditor::resized()
{
    auto r = getLocalBounds().withTrimmedLeft (kCheek).withTrimmedRight (kCheek);
    auto top = r.removeFromTop (kTop).reduced (6, 6); top.removeFromLeft (130);
    saveBtn.setBounds (top.removeFromRight (70)); top.removeFromRight (6); renameBtn.setBounds (top.removeFromRight (80)); top.removeFromRight (6); nextBtn.setBounds (top.removeFromRight (34)); top.removeFromRight (4);
    prevBtn.setBounds (top.removeFromLeft (34)); top.removeFromLeft (4); presetBox.setBounds (top);
    meterArea = r.removeFromRight (kMeterW); generic.setBounds (r);
}
