#include "PluginEditor.h"
#include "ClipboardWin.h"

using namespace fg;
using namespace fg::ui;

// ------------------------------------------------------------------ Hintergrund
void FlipGrainEditor::Backdrop::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (col::bg1, 0.f, 0.f, col::bg0, 0.f, b.getHeight(), false));
    g.fillAll();

    // Aurora-Glows
    g.setGradientFill (juce::ColourGradient (col::cyan.withAlpha (0.10f), 120.f, -40.f, col::cyan.withAlpha (0.f), 120.f, 330.f, true));
    g.fillRect (b);
    g.setGradientFill (juce::ColourGradient (col::magenta.withAlpha (0.09f), b.getRight() - 80.f, -30.f,
                                             col::magenta.withAlpha (0.f), b.getRight() - 80.f, 340.f, true));
    g.fillRect (b);
    g.setGradientFill (juce::ColourGradient (col::violet.withAlpha (0.07f), b.getCentreX(), b.getBottom() + 40.f,
                                             col::violet.withAlpha (0.f), b.getCentreX(), b.getBottom() - 330.f, true));
    g.fillRect (b);

    // feines Raster
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (int x = 0; x < getWidth(); x += 24) g.drawVerticalLine (x, 0.f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.drawHorizontalLine (y, 0.f, (float) getWidth());

    // --- Header
    {   // Logo-Glyph: 5 Balken, Mitte "geflippt"
        const float lx = 28.f, ly = 18.f;
        const float hs[5] = { 10.f, 22.f, 30.f, 16.f, 24.f };
        for (int i = 0; i < 5; ++i)
        {
            const auto c = col::cyan.interpolatedWith (col::magenta, (float) i / 4.f);
            g.setColour (c.withAlpha (0.25f));
            g.fillRoundedRectangle (lx + (float) i * 8.f - 2.f, ly + 15.f - hs[i] * 0.5f - 2.f, 8.f, hs[i] + 4.f, 3.f);
            g.setColour (c);
            g.fillRoundedRectangle (lx + (float) i * 8.f, ly + 15.f - hs[i] * 0.5f, 4.f, hs[i], 2.f);
        }
    }
    g.setGradientFill (juce::ColourGradient (col::cyan, 82.f, 0.f, col::magenta, 260.f, 0.f, false));
    g.setFont (F (27.f, true).withExtraKerningFactor (0.16f));
    g.drawText ("FLIPGRAIN", juce::Rectangle<int> (80, 10, 240, 34), juce::Justification::centredLeft, false);
    g.setColour (col::dim);
    g.setFont (F (9.5f, true).withExtraKerningFactor (0.18f));
    g.drawText ("SAMPLE FLIPPER  -  GRANULAR MELODY ENGINE", juce::Rectangle<int> (82, 40, 360, 14), juce::Justification::centredLeft, false);

    if (status.isNotEmpty())
    {
        auto r = juce::Rectangle<float> ((float) getWidth() * 0.5f - 190.f, 17.f, 380.f, 28.f);
        g.setColour (col::amber.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 14.f);
        g.setColour (col::amber.withAlpha (0.9f));
        g.drawRoundedRectangle (r, 14.f, 1.f);
        g.setFont (F (12.5f, true));
        g.drawText (status, r, juce::Justification::centred, false);
    }

    g.setColour (col::faint);
    g.setFont (F (10.f, true).withExtraKerningFactor (0.1f));
    g.drawText ("v1.0.0", juce::Rectangle<int> (getWidth() - 120, 44, 96, 14), juce::Justification::centredRight, false);

    // --- Panels
    for (auto& p : panels)
    {
        panelBackground (g, p.r, 16.f);
        g.setColour (p.accent);
        g.fillEllipse (p.r.getX() + 16.f, p.r.getY() + 15.f, 7.f, 7.f);
        g.setColour (p.accent.withAlpha (0.25f));
        g.fillEllipse (p.r.getX() + 12.f, p.r.getY() + 11.f, 15.f, 15.f);
        g.setColour (col::text);
        g.setFont (F (12.f, true).withExtraKerningFactor (0.2f));
        g.drawText (p.title, juce::Rectangle<float> (p.r.getX() + 32.f, p.r.getY() + 8.f, 260.f, 20.f),
                    juce::Justification::centredLeft, false);
    }
    g.setColour (col::dim);
    g.setFont (F (9.5f, true).withExtraKerningFactor (0.14f));
    for (auto& c : captions) g.drawText (c.text, c.r, juce::Justification::centredLeft, false);
}

// ------------------------------------------------------------------ Editor
FlipGrainEditor::FlipGrainEditor (FlipGrainProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveform (p), keyboard (p), grainView (p.apvts)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (root);
    layoutAll();

    setResizable (true, true);
    setResizeLimits (kW * 7 / 10, kH * 7 / 10, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    setWantsKeyboardFocus (true);
    startTimerHz (30);
}

FlipGrainEditor::~FlipGrainEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FlipGrainEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    root.setTransform (juce::AffineTransform::scale (s));
    root.setBounds (0, 0, kW, kH);
}

void FlipGrainEditor::layoutAll()
{
    auto& ap = proc.apvts;

    auto addKnob = [&] (const char* pid, const char* label, int x, int y, int w, juce::Colour c, bool bip = false)
    {
        auto k = std::make_unique<Knob> (ap, pid, label, c, bip);
        k->setBounds (x, y, w, 82);
        root.addAndMakeVisible (*k);
        knobs.push_back (std::move (k));
    };
    auto addSeg = [&] (const char* pid, juce::StringArray labels, juce::Rectangle<int> r, juce::Colour c)
    {
        auto s = std::make_unique<Segmented> (ap, pid, labels, c);
        s->setBounds (r);
        root.addAndMakeVisible (*s);
        segs.push_back (std::move (s));
    };
    auto fillCombo = [&] (juce::ComboBox& box, const char* pid)
    {
        if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (ap.getParameter (pid)))
            for (int i = 0; i < ch->choices.size(); ++i) box.addItem (ch->choices[i], i + 1);
    };

    // ---- Panels
    root.panels = {
        { { 24.f, 352.f, 372.f, 284.f },  "MELODY FLIP",  col::magenta },
        { { 408.f, 352.f, 404.f, 284.f }, "GRANULAR",     col::cyan },
        { { 824.f, 352.f, 332.f, 284.f }, "SHAPE & FX",   col::violet },
    };

    // ---- Waveform + Header
    waveform.setBounds (24, 76, 1132, 262);
    root.addAndMakeVisible (waveform);
    meter.setBounds (1126, 14, 30, 34);
    root.addAndMakeVisible (meter);

    loadButton.setBounds (1156 - 16 - 88 - 8 - 82, 84 + 4, 82, 26);
    pasteButton.setBounds (1156 - 16 - 88, 84 + 4, 88, 26);
    root.addAndMakeVisible (loadButton);
    root.addAndMakeVisible (pasteButton);
    loadButton.onClick = [this] { chooseFile(); };
    pasteButton.onClick = [this] { pasteFromClipboard(); };

    // ---- Melody Flip panel
    addSeg (PID::mode,   { "CHOP", "KEYS" }, { 40, 386, 128, 28 }, col::magenta);
    addSeg (PID::engine, { "GRANULAR", "CLASSIC" }, { 178, 386, 202, 28 }, col::cyan);

    root.captions.push_back ({ "KEY",    { 42.f, 424.f, 80.f, 12.f } });
    root.captions.push_back ({ "SCALE",  { 146.f, 424.f, 80.f, 12.f } });
    root.captions.push_back ({ "SLICES", { 306.f, 424.f, 80.f, 12.f } });
    keyBox.setBounds (40, 438, 98, 28);     fillCombo (keyBox, PID::key);
    scaleBox.setBounds (146, 438, 150, 28); fillCombo (scaleBox, PID::scale);
    sliceBox.setBounds (304, 438, 76, 28);  fillCombo (sliceBox, PID::sliceMode);
    keyAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (ap, PID::key, keyBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (ap, PID::scale, scaleBox);
    sliceAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (ap, PID::sliceMode, sliceBox);
    root.addAndMakeVisible (keyBox); root.addAndMakeVisible (scaleBox); root.addAndMakeVisible (sliceBox);

    addKnob (PID::retune,  "Retune",   38,  474, 86, col::magenta);
    addKnob (PID::flipAmt, "Flip Amt", 124, 474, 86, col::magenta);
    addKnob (PID::sens,    "Sens",     210, 474, 86, col::amber);
    addKnob (PID::region,  "Region",   296, 474, 86, col::cyan);

    flipButton.setBounds (36, 568, 176, 58);
    root.addAndMakeVisible (flipButton);
    flipButton.onClick = [this]
    {
        if (! proc.getBank()) { toast ("Load a sample first"); return; }
        proc.flipSample();
    };
    resetButton.setBounds (220, 574, 66, 46);
    root.addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { proc.resetFlip(); };

    midiPad.setBounds (292, 568, 94, 58);
    root.addAndMakeVisible (midiPad);
    midiPad.makeTempFile = [this]
    {
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory)
                     .getChildFile ("FlipGrain_Pattern_" + juce::String (++tempCounter % 8) + ".mid");
        return proc.exportPatternTo (f) ? f : juce::File();
    };
    midiPad.onClickSave = [this]
    {
        if (! proc.getBank()) { toast ("Load a sample first"); return; }
        chooser = std::make_unique<juce::FileChooser> ("Save pattern", juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                                                                           .getChildFile ("FlipGrain_Pattern.mid"), "*.mid");
        juce::Component::SafePointer<FlipGrainEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr) return;
                                  auto f = fc.getResult();
                                  if (f != juce::File()) safe->proc.exportPatternTo (f.withFileExtension ("mid"));
                              });
    };

    // ---- Granular panel
    const int gx = 420, gw = 76;
    addKnob (PID::position,  "Position", gx + gw * 0, 386, gw, col::cyan);
    addKnob (PID::scan,      "Scan",     gx + gw * 1, 386, gw, col::cyan, true);
    addKnob (PID::grainSize, "Size",     gx + gw * 2, 386, gw, col::cyan);
    addKnob (PID::density,   "Density",  gx + gw * 3, 386, gw, col::cyan);
    addKnob (PID::spray,     "Spray",    gx + gw * 4, 386, gw, col::cyan);
    addKnob (PID::jitter,    "Jitter",   gx + gw * 0, 470, gw, col::violet);
    addKnob (PID::spread,    "Spread",   gx + gw * 1, 470, gw, col::violet);
    addKnob (PID::revProb,   "Reverse",  gx + gw * 2, 470, gw, col::violet);
    addKnob (PID::shape,     "Shape",    gx + gw * 3, 470, gw, col::violet);
    addSeg  (PID::loop, { "ONCE", "LOOP" }, { gx + gw * 4 + 2, 494, gw - 4, 26 }, col::green);
    grainView.setBounds (420, 556, 380, 70);
    root.addAndMakeVisible (grainView);

    // ---- Shape & FX panel
    const int fx = 834, fw = 78;
    addKnob (PID::attack,  "Attack",  fx + fw * 0, 386, fw, col::amber);
    addKnob (PID::decay,   "Decay",   fx + fw * 1, 386, fw, col::amber);
    addKnob (PID::sustain, "Sustain", fx + fw * 2, 386, fw, col::amber);
    addKnob (PID::release, "Release", fx + fw * 3, 386, fw, col::amber);
    addKnob (PID::lp,      "Lowpass", fx + fw * 0, 470, fw, col::violet);
    addKnob (PID::hp,      "Highpass", fx + fw * 1, 470, fw, col::violet);
    addKnob (PID::res,     "Reso",    fx + fw * 2, 470, fw, col::violet);
    addKnob (PID::tune,    "Tune",    fx + fw * 3, 470, fw, col::green, true);
    addKnob (PID::revMix,  "Reverb",  fx + fw * 0, 552, fw, col::cyan);
    addKnob (PID::revSize, "Space",   fx + fw * 1, 552, fw, col::cyan);
    addKnob (PID::base,    "Root",    fx + fw * 2, 552, fw, col::green);
    addKnob (PID::gain,    "Gain",    fx + fw * 3, 552, fw, col::magenta, true);

    // ---- Keyboard
    keyboard.setBounds (24, 648, 1132, 96);
    root.addAndMakeVisible (keyboard);

    for (auto* b : { &loadButton, &pasteButton, &resetButton }) b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

// ------------------------------------------------------------------ Laden / Einfuegen
bool FlipGrainEditor::isAudioFile (const juce::File& f)
{
    static const juce::StringArray ext { ".wav", ".wave", ".aif", ".aiff", ".flac", ".mp3", ".ogg", ".m4a", ".wma" };
    return f.existsAsFile() && ext.contains (f.getFileExtension().toLowerCase());
}

void FlipGrainEditor::tryLoad (const juce::File& f)
{
    if (isAudioFile (f)) proc.loadSampleFile (f);
    else toast ("Not an audio file");
}

void FlipGrainEditor::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a");
    juce::Component::SafePointer<FlipGrainEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr) return;
                              auto f = fc.getResult();
                              if (f.existsAsFile()) safe->tryLoad (f);
                          });
}

void FlipGrainEditor::pasteFromClipboard()
{
    for (auto& w : fg::getClipboardFilePaths())
    {
        juce::File f { juce::String (w.c_str()) };
        if (isAudioFile (f)) { proc.loadSampleFile (f); return; }
    }
    juce::File f (juce::SystemClipboard::getTextFromClipboard().trim().unquoted());
    if (isAudioFile (f)) { proc.loadSampleFile (f); return; }
    toast ("Clipboard has no audio file - copy a file in Explorer first");
}

bool FlipGrainEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('v', juce::ModifierKeys::ctrlModifier, 0)
        || k == juce::KeyPress ('v', juce::ModifierKeys::commandModifier, 0))
    {
        pasteFromClipboard();
        return true;
    }
    return false;
}

bool FlipGrainEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (isAudioFile (juce::File (f))) return true;
    return false;
}
void FlipGrainEditor::filesDropped (const juce::StringArray& files, int, int)
{
    waveform.setDragOver (false);
    for (auto& f : files) if (isAudioFile (juce::File (f))) { proc.loadSampleFile (juce::File (f)); return; }
}
void FlipGrainEditor::fileDragEnter (const juce::StringArray&, int, int) { waveform.setDragOver (true); }
void FlipGrainEditor::fileDragExit (const juce::StringArray&) { waveform.setDragOver (false); }

void FlipGrainEditor::toast (const juce::String& t)
{
    toastText = t;
    toastUntil = juce::Time::getMillisecondCounter() + 2800;
}

void FlipGrainEditor::timerCallback()
{
    meter.push (proc.meterL.exchange (0.f), proc.meterR.exchange (0.f));

    juce::String st = proc.getStatus();
    if (st.isEmpty() && proc.isLoading()) st = "Analysing ...";
    if (juce::Time::getMillisecondCounter() < toastUntil) st = toastText;
    if (st != root.status) { root.status = st; root.repaint (root.getLocalBounds().removeFromTop (70)); }

    auto b = proc.getBank();
    if (b != lastBank) { lastBank = b; midiPad.setBank (b); }
}
