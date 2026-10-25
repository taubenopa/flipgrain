#pragma once
#include "Theme.h"
#include "../Params.h"
#include "../Bank.h"

namespace fg::ui
{
// ---------------------------------------------------------------- LookAndFeel
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel()
    {
        setColour (juce::ComboBox::textColourId, col::text);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf0101524));
        setColour (juce::PopupMenu::textColourId, col::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::cyan.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TextButton::textColourOffId, col::text);
        setColour (juce::TextButton::textColourOnId, col::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
        const float d = std::min (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float r = d * 0.5f;
        const float arcR = r - 3.f;
        const float ang = a0 + pos * (a1 - a0);
        const bool bip = (bool) s.getProperties()["bipolar"];
        const float zero = a0 + (a1 - a0) * 0.5f;
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
        const bool hot = s.isMouseOverOrDragging();

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, a0, a1, true);
        g.setColour (juce::Colour (0xff1b2238));
        g.strokePath (track, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float from = bip ? zero : a0;
        if (std::abs (ang - from) > 0.002f)
        {
            juce::Path v;
            v.addCentredArc (c.x, c.y, arcR, arcR, 0.f, std::min (from, ang), std::max (from, ang), true);
            for (int k = 3; k >= 1; --k)
            {
                g.setColour (accent.withAlpha ((hot ? 0.10f : 0.06f) * (float) (4 - k)));
                g.strokePath (v, juce::PathStrokeType (4.f + (float) k * 3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setGradientFill (juce::ColourGradient (accent.brighter (0.2f), c.x - r, c.y,
                                                     accent.interpolatedWith (col::magenta, 0.55f), c.x + r, c.y, false));
            g.strokePath (v, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float br = r - 9.f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a3350), c.x, c.y - br,
                                                 juce::Colour (0xff10152a), c.x, c.y + br, false));
        g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (juce::Colours::white.withAlpha (hot ? 0.28f : 0.14f));
        g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.f);

        const float sx = std::sin (ang), sy = -std::cos (ang);
        g.setColour (accent.brighter (0.4f));
        g.drawLine (c.x + sx * br * 0.40f, c.y + sy * br * 0.40f,
                    c.x + sx * br * 0.92f, c.y + sy * br * 0.92f, 2.4f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool down, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setColour (juce::Colour (0xff141a2c));
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (down || box.isMouseOver() ? col::cyan.withAlpha (0.8f) : col::border);
        g.drawRoundedRectangle (r, 8.f, 1.f);
        juce::Path chev;
        const float cx = (float) w - 15.f, cy = (float) h * 0.5f;
        chev.startNewSubPath (cx - 4.f, cy - 2.f); chev.lineTo (cx, cy + 2.f); chev.lineTo (cx + 4.f, cy - 2.f);
        g.setColour (col::cyan);
        g.strokePath (chev, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return F (13.f, true); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }
    juce::Font getPopupMenuFont() override { return F (13.5f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (juce::Colour (0xff0e1220));
        g.setColour (col::border);
        g.drawRect (0, 0, w, h, 1);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? col::cyan.withAlpha (0.28f) : (hover ? col::cyan.withAlpha (0.14f) : juce::Colour (0xff141a2c)));
        g.fillRoundedRectangle (r, 9.f);
        g.setColour (hover ? col::cyan.withAlpha (0.9f) : col::border);
        g.drawRoundedRectangle (r, 9.f, 1.f);
    }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return F (12.f, true).withExtraKerningFactor (0.1f); }
};

// ---------------------------------------------------------------- Knob
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const char* paramID, const juce::String& label,
          juce::Colour accent = col::cyan, bool bipolar = false)
        : name (label.toUpperCase())
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.getProperties().set ("bipolar", bipolar);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramID, slider);
        if (auto* prm = apvts.getParameter (paramID))
        {
            unit = prm->getLabel();
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        }
        slider.addMouseListener (this, false);
    }

    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (18)); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().removeFromBottom (18);
        const bool show = slider.isMouseOverOrDragging();
        juce::String t = name;
        if (show)
        {
            t = slider.getTextFromValue (slider.getValue());
            if (unit.isNotEmpty() && unit != "%") t << " " << unit;
            if (unit == "%") t << "%";
        }
        g.setFont (F (show ? 12.5f : 10.5f, true).withExtraKerningFactor (show ? 0.f : 0.08f));
        g.setColour (show ? col::text : col::dim);
        g.drawText (t, r, juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDrag (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    juce::String name, unit;
};

// ---------------------------------------------------------------- Segmented (Choice / Bool)
class Segmented : public juce::Component
{
public:
    Segmented (juce::AudioProcessorValueTreeState& apvts, const char* pid, juce::StringArray labelsIn,
               juce::Colour accentIn = col::cyan)
        : labels (std::move (labelsIn)), accent (accentIn),
          att (*apvts.getParameter (pid), [this] (float v) { index = (int) std::lround (v); repaint(); })
    {
        att.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xff0d1120));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (col::border);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.f);
        const float w = r.getWidth() / (float) labels.size();
        for (int i = 0; i < labels.size(); ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.5f);
            const bool on = i == index;
            if (on)
            {
                g.setGradientFill (juce::ColourGradient (accent, seg.getX(), seg.getY(),
                                                         accent.interpolatedWith (col::violet, 0.6f), seg.getRight(), seg.getBottom(), false));
                g.fillRoundedRectangle (seg, seg.getHeight() * 0.5f);
            }
            g.setColour (on ? juce::Colour (0xff070914) : col::dim);
            g.setFont (F (11.f, true).withExtraKerningFactor (0.08f));
            g.drawText (labels[i], seg, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, labels.size() - 1, (int) ((float) e.x / ((float) getWidth() / (float) labels.size())));
        att.setValueAsCompleteGesture ((float) i);
    }

private:
    juce::StringArray labels;
    juce::Colour accent;
    int index = 0;
    juce::ParameterAttachment att;
};

// ---------------------------------------------------------------- Big FLIP button
class FlipButton : public juce::Component, private juce::Timer
{
public:
    std::function<void()> onClick;
    FlipButton() { startTimerHz (60); setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (4.f);
        const float rad = b.getHeight() * 0.5f;
        for (int k = 4; k >= 1; --k)
        {
            g.setColour (col::magenta.withAlpha ((0.035f + 0.05f * pulse) * (float) (5 - k)));
            g.fillRoundedRectangle (b.expanded ((float) k * 2.5f), rad + (float) k * 2.5f);
        }
        const float sh = 0.5f + 0.5f * std::sin (phase);
        juce::ColourGradient grad (col::cyan, b.getX(), b.getY(), col::magenta, b.getRight(), b.getBottom(), false);
        grad.addColour (juce::jlimit (0.2, 0.8, (double) sh), col::violet);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, rad);
        if (hover || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.18f));
            g.fillRoundedRectangle (b, rad);
        }
        {   // Glanz-Streifen
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (b, rad);
            g.reduceClipRegion (clip);
            const float sx = b.getX() - 60.f + std::fmod (phase * 40.f, b.getWidth() + 160.f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.f), sx, 0.f,
                                                     juce::Colours::white.withAlpha (0.35f), sx + 30.f, 0.f, false));
            g.fillRect (sx, b.getY(), 60.f, b.getHeight());
        }
        // Wuerfel-Icon
        auto die = juce::Rectangle<float> (b.getX() + 22.f, b.getCentreY() - 12.f, 24.f, 24.f);
        g.setColour (juce::Colour (0xff070914));
        g.fillRoundedRectangle (die, 6.f);
        g.setColour (juce::Colours::white);
        const float px[5] = { 0.27f, 0.73f, 0.5f, 0.27f, 0.73f }, py[5] = { 0.27f, 0.27f, 0.5f, 0.73f, 0.73f };
        for (int i = 0; i < 5; ++i)
            g.fillEllipse (die.getX() + die.getWidth() * px[i] - 2.2f, die.getY() + die.getHeight() * py[i] - 2.2f, 4.4f, 4.4f);
        g.setColour (juce::Colour (0xff070914));
        g.setFont (F (22.f, true).withExtraKerningFactor (0.18f));
        g.drawText ("FLIP", b.withTrimmedLeft (50.f), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; }
    void mouseExit (const juce::MouseEvent&) override { hover = false; down = false; }
    void mouseDown (const juce::MouseEvent&) override { down = true; }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false;
        if (contains (e.getPosition()) && onClick) { pulse = 1.f; onClick(); }
    }

private:
    void timerCallback() override
    {
        phase += 0.03f + pulse * 0.25f;
        pulse *= 0.93f;
        repaint();
    }
    float phase = 0.f, pulse = 0.f;
    bool hover = false, down = false;
};

// ---------------------------------------------------------------- MIDI drag pad
class MidiPad : public juce::Component
{
public:
    std::function<juce::File()> makeTempFile;
    std::function<void()> onClickSave;

    void setBank (std::shared_ptr<const Bank> b) { bank = std::move (b); repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (3.f);
        g.setColour (juce::Colour (0xff0d1120));
        g.fillRoundedRectangle (r, 12.f);
        g.setColour (hover ? col::cyan.withAlpha (0.9f) : col::border);
        g.drawRoundedRectangle (r, 12.f, 1.f);

        auto pr = r.reduced (8.f, 6.f).withTrimmedBottom (13.f);
        if (bank && ! bank->pattern.empty())
        {
            const int n = std::max (1, bank->numSlices());
            for (auto& pn : bank->pattern)
            {
                const float x = pr.getX() + pr.getWidth() * (float) pn.step / 32.f;
                const float w = std::max (2.f, pr.getWidth() * (float) pn.length / 32.f - 1.f);
                const float y = pr.getBottom() - pr.getHeight() * ((float) (pn.key + 1) / (float) (n + 1));
                const int sl = pn.key < (int) bank->order.size() ? bank->order[(size_t) pn.key] : pn.key;
                g.setColour (sliceColour (sl, n).withAlpha (0.9f));
                g.fillRoundedRectangle (x, y - 1.5f, w, 3.f, 1.5f);
            }
        }
        g.setColour (hover ? col::text : col::dim);
        g.setFont (F (9.5f, true).withExtraKerningFactor (0.1f));
        g.drawText ("DRAG MIDI", r.withTrimmedTop (r.getHeight() - 15.f), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeTempFile) return;
        auto f = makeTempFile();
        if (f.existsAsFile())
        {
            dragging = true;
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
        }
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging && onClickSave) onClickSave();
        dragging = false;
    }

private:
    std::shared_ptr<const Bank> bank;
    bool hover = false, dragging = false;
};

// ---------------------------------------------------------------- Output meter
class MeterView : public juce::Component
{
public:
    void push (float l, float r)
    {
        lv = std::max (l, lv * 0.86f); rv = std::max (r, rv * 0.86f);
        lp = std::max (lv, lp * 0.985f); rp = std::max (rv, rp * 0.985f);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float w = (r.getWidth() - 4.f) * 0.5f;
        drawBar (g, { r.getX(), r.getY(), w, r.getHeight() }, lv, lp);
        drawBar (g, { r.getX() + w + 4.f, r.getY(), w, r.getHeight() }, rv, rp);
    }
private:
    static void drawBar (juce::Graphics& g, juce::Rectangle<float> r, float v, float peak)
    {
        g.setColour (juce::Colour (0xff10152a));
        g.fillRoundedRectangle (r, 2.f);
        const float h = r.getHeight() * std::pow (juce::jlimit (0.f, 1.f, v), 0.6f);
        g.setGradientFill (juce::ColourGradient (col::magenta, 0.f, r.getY(), col::cyan, 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r.withTop (r.getBottom() - h), 2.f);
        const float py = r.getBottom() - r.getHeight() * std::pow (juce::jlimit (0.f, 1.f, peak), 0.6f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillRect (r.getX(), py, r.getWidth(), 1.5f);
    }
    float lv = 0, rv = 0, lp = 0, rp = 0;
};

// ---------------------------------------------------------------- Grain shape / density preview
class GrainView : public juce::Component, private juce::Timer
{
public:
    explicit GrainView (juce::AudioProcessorValueTreeState& a) : apvts (a) { startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0b0f1c));
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (col::border);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f);

        const float sizeMs = apvts.getRawParameterValue (PID::grainSize)->load() * 1000.f;
        const float dens   = apvts.getRawParameterValue (PID::density)->load();
        const float shape  = apvts.getRawParameterValue (PID::shape)->load();
        const float spray  = apvts.getRawParameterValue (PID::spray)->load();
        const float fade   = juce::jmap (shape, 0.05f, 0.5f);

        auto pr = r.reduced (10.f, 8.f);
        const float spanMs = juce::jlimit (80.f, 1600.f, std::max (sizeMs * 2.4f, 1000.f / dens * 6.f));
        const float interval = 1000.f / dens;
        const int lanes = 4;
        int k = 0;
        for (float t = 0.f; t < spanMs && k < 64; t += interval, ++k)
        {
            const float jit = std::sin ((float) k * 7.31f) * spray * interval * 0.4f;
            const float x0 = pr.getX() + pr.getWidth() * juce::jlimit (0.f, 1.f, (t + jit) / spanMs);
            const float wpx = pr.getWidth() * sizeMs / spanMs;
            const float lane = (float) (k % lanes);
            const float laneH = pr.getHeight() / (float) lanes;
            const float base = pr.getBottom() - lane * laneH * 0.55f;
            const float hgt = pr.getHeight() * 0.62f;
            juce::Path p;
            p.startNewSubPath (x0, base);
            const int steps = 28;
            for (int s = 0; s <= steps; ++s)
            {
                const float u = (float) s / (float) steps;
                float w = 1.f;
                if (u < fade) w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * u / fade);
                else if (u > 1.f - fade) w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (1.f - u) / fade);
                p.lineTo (x0 + wpx * u, base - hgt * w);
            }
            p.lineTo (x0 + wpx, base);
            p.closeSubPath();
            const auto c = juce::Colour::fromHSV (0.5f + 0.4f * (float) (k % lanes) / (float) lanes, 0.6f, 1.f, 1.f);
            g.setColour (c.withAlpha (0.16f));
            g.fillPath (p);
            g.setColour (c.withAlpha (0.55f));
            g.strokePath (p, juce::PathStrokeType (1.f));
        }
        g.setColour (col::dim);
        g.setFont (F (10.f, true).withExtraKerningFactor (0.08f));
        g.drawText ("GRAIN CLOUD  -  " + juce::String (juce::roundToInt (sizeMs)) + " ms  x  "
                    + juce::String (juce::roundToInt (dens)) + "/s  =  "
                    + juce::String (sizeMs * 0.001f * dens, 1) + " overlap",
                    r.reduced (12.f, 5.f).removeFromTop (14.f), juce::Justification::topLeft, false);
    }
private:
    void timerCallback() override { repaint(); }
    juce::AudioProcessorValueTreeState& apvts;
};
} // namespace fg::ui
