#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"

namespace fg::ui
{
// ============================================================== Waveform
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (FlipGrainProcessor& p) : proc (p) { startTimerHz (30); }

    void setDragOver (bool b) { dragOver = b; repaint(); }
    std::shared_ptr<const Bank> getShownBank() const { return bank; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        panelBackground (g, b, 16.f);
        if (dragOver)
        {
            g.setColour (col::cyan.withAlpha (0.08f));
            g.fillRoundedRectangle (b, 16.f);
            g.setColour (col::cyan.withAlpha (0.9f));
            g.drawRoundedRectangle (b.reduced (1.5f), 16.f, 2.f);
        }
        const auto wr = waveRect();
        const float cy = wr.getCentreY();
        g.setColour (col::faint.withAlpha (0.28f));
        g.drawHorizontalLine ((int) cy, wr.getX(), wr.getRight());
        for (int k = 1; k <= 2; ++k)
        {
            g.setColour (col::faint.withAlpha (0.10f));
            g.drawHorizontalLine ((int) (cy - (float) k * wr.getHeight() / 6.f), wr.getX(), wr.getRight());
            g.drawHorizontalLine ((int) (cy + (float) k * wr.getHeight() / 6.f), wr.getX(), wr.getRight());
        }

        if (! bank)
        {
            drawEmpty (g, b);
            return;
        }

        const int n = bank->numSlices();
        const int len = bank->audio->getNumSamples();
        const float hh = wr.getHeight() * 0.5f * 0.92f;
        const uint64_t mask = proc.activeSliceMask.load (std::memory_order_relaxed);
        const int baseNote = (int) proc.apvts.getRawParameterValue (PID::base)->load();
        const bool chop = (int) proc.apvts.getRawParameterValue (PID::mode)->load() == 0;
        const int cols = (int) hiPk.size();

        for (int i = 0; i < n; ++i)
        {
            const auto& s = bank->slices[(size_t) i];
            const float x0 = wr.getX() + wr.getWidth() * (float) s.start / (float) len;
            const float x1 = wr.getX() + wr.getWidth() * (float) s.end / (float) len;
            const auto c = sliceColour (i, n);
            const bool act = i < 64 && ((mask >> i) & 1u) != 0;

            g.setGradientFill (juce::ColourGradient (c.withAlpha (act ? 0.34f : 0.08f), 0.f, wr.getY(),
                                                     c.withAlpha (act ? 0.10f : 0.01f), 0.f, wr.getBottom(), false));
            g.fillRect (juce::Rectangle<float> (x0, wr.getY(), x1 - x0, wr.getHeight()));

            int c0 = juce::jlimit (0, cols - 1, (int) (x0 - wr.getX()));
            int c1 = juce::jlimit (0, cols - 1, (int) (x1 - wr.getX()));
            if (c1 > c0 && cols > 0)
            {
                juce::Path p;
                p.startNewSubPath (wr.getX() + (float) c0, cy - hiPk[(size_t) c0] * hh);
                for (int x = c0 + 1; x <= c1; ++x) p.lineTo (wr.getX() + (float) x, cy - hiPk[(size_t) x] * hh);
                for (int x = c1; x >= c0; --x)     p.lineTo (wr.getX() + (float) x, cy - loPk[(size_t) x] * hh);
                p.closeSubPath();
                g.setColour (act ? c.brighter (0.3f) : c.withAlpha (0.82f));
                g.fillPath (p);
            }

            g.setColour (c.withAlpha (0.75f));
            g.fillRect (x0, wr.getY(), 1.5f, wr.getHeight());

            // Tasten-Tag oben
            if (chop)
            {
                const int k = i < (int) bank->orderInv.size() ? bank->orderInv[(size_t) i] : i;
                const float tw = std::min (30.f, x1 - x0 - 5.f);
                if (tw > 10.f)
                {
                    juce::Rectangle<float> tag (x0 + 3.f, wr.getY() + 3.f, tw, 15.f);
                    g.setColour (c.withAlpha (0.92f));
                    g.fillRoundedRectangle (tag, 4.f);
                    if (tw >= 22.f)
                    {
                        g.setColour (juce::Colour (0xff070914));
                        g.setFont (F (10.f, true));
                        g.drawText (music::noteName (baseNote + k), tag, juce::Justification::centred, false);
                    }
                }
            }

            // Tonhoehe unten
            const float sw = x1 - x0;
            if (sw > 40.f)
            {
                juce::String t = "-";
                if (s.pitched)
                {
                    const float semis = proc.getSliceSemis (*bank, i);
                    t = music::noteName ((int) std::lround (s.midi)) + " > " + music::noteName ((int) std::lround (s.midi + semis));
                }
                if (s.reverse) t = juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x82 ")) + t;
                g.setColour (s.pitched ? c.brighter (0.2f) : col::dim);
                g.setFont (F (10.f, true));
                g.drawText (t, juce::Rectangle<float> (x0 + 4.f, wr.getBottom() + 3.f, sw - 6.f, 16.f),
                            juce::Justification::centredLeft, false);
            }
        }

        // Playheads
        for (auto& ph : proc.playheads)
        {
            const float v = ph.load (std::memory_order_relaxed);
            if (v < 0.f) continue;
            const float x = wr.getX() + wr.getWidth() * v;
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.f), x - 8.f, 0.f,
                                                     juce::Colours::white.withAlpha (0.22f), x, 0.f, false));
            g.fillRect (x - 8.f, wr.getY(), 8.f, wr.getHeight());
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.fillRect (x - 0.75f, wr.getY(), 1.5f, wr.getHeight());
        }

        // Grains
        for (int i = 0; i < FlipGrainProcessor::kViz; ++i)
        {
            const float a = proc.vizA[(size_t) i].load (std::memory_order_relaxed);
            if (a < 0.02f) continue;
            const float x = wr.getX() + wr.getWidth() * proc.vizX[(size_t) i].load (std::memory_order_relaxed);
            const float hsh = std::abs (std::fmod (std::sin ((float) i * 12.9898f) * 43758.5453f, 1.f));
            const float y = cy + (hsh - 0.5f) * hh * 1.5f;
            const float rad = 2.f + a * 4.f;
            g.setColour (col::cyan.withAlpha (0.14f * a));
            g.fillEllipse (x - rad * 2.2f, y - rad * 2.2f, rad * 4.4f, rad * 4.4f);
            g.setColour (juce::Colours::white.withAlpha (0.9f * a));
            g.fillEllipse (x - rad * 0.5f, y - rad * 0.5f, rad, rad);
        }

        drawHeader (g, b);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int k = keyAt (e.position);
        if (k >= 0)
        {
            heldNote = (int) proc.apvts.getRawParameterValue (PID::base)->load() + k;
            proc.keyboardState.noteOn (1, heldNote, 0.85f);
        }
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (heldNote >= 0) { proc.keyboardState.noteOff (1, heldNote, 0.f); heldNote = -1; }
    }

private:
    juce::Rectangle<float> waveRect() const
    {
        return { 16.f, 44.f, (float) getWidth() - 32.f, (float) getHeight() - 44.f - 28.f };
    }

    int keyAt (juce::Point<float> pos) const
    {
        if (! bank || bank->numSlices() == 0) return -1;
        const auto wr = waveRect();
        if (! wr.expanded (0.f, 20.f).contains (pos)) return -1;
        const int sample = (int) ((pos.x - wr.getX()) / wr.getWidth() * (float) bank->audio->getNumSamples());
        for (int i = 0; i < bank->numSlices(); ++i)
            if (sample >= bank->slices[(size_t) i].start && sample < bank->slices[(size_t) i].end)
                return bank->orderInv[(size_t) i];
        return -1;
    }

    void drawEmpty (juce::Graphics& g, juce::Rectangle<float> b)
    {
        const float t = (float) juce::Time::getMillisecondCounterHiRes() * 0.002f;
        const float pulse = 0.5f + 0.5f * std::sin (t);
        auto r = b.reduced (28.f, 24.f);
        juce::Path rr, dashed;
        rr.addRoundedRectangle (r, 14.f);
        const float dashes[] = { 7.f, 6.f };
        juce::PathStrokeType (1.6f).createDashedStroke (dashed, rr, dashes, 2);
        g.setColour (col::cyan.withAlpha (0.25f + 0.25f * pulse));
        g.fillPath (dashed);

        // Deko-Waveform
        const float cy = b.getCentreY() + 40.f;
        g.setColour (col::faint.withAlpha (0.35f));
        for (int x = 0; x < (int) r.getWidth() - 40; x += 6)
        {
            const float env = std::sin ((float) x * 0.012f) * std::sin ((float) x * 0.0037f + 1.f);
            const float h = (4.f + 22.f * std::abs (env) * (0.6f + 0.4f * std::sin ((float) x * 0.31f))) ;
            g.fillRoundedRectangle (r.getX() + 20.f + (float) x, cy - h * 0.5f, 3.f, h, 1.5f);
        }
        g.setColour (col::text);
        g.setFont (F (26.f, true).withExtraKerningFactor (0.14f));
        g.drawText (proc.isLoading() ? "ANALYSING ..." : "DROP A SAMPLE HERE", b.withTrimmedBottom (40.f), juce::Justification::centred, false);
        g.setColour (col::dim);
        g.setFont (F (13.f));
        g.drawText ("or press  Ctrl + V  to paste a copied audio file   -   or click LOAD",
                    b.withTrimmedTop (b.getHeight() * 0.5f - 10.f).withHeight (24.f), juce::Justification::centred, false);
    }

    void drawHeader (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.setColour (col::text);
        g.setFont (F (15.f, true));
        g.drawText (bank->name.toUpperCase(), juce::Rectangle<float> (18.f, 10.f, 420.f, 22.f), juce::Justification::centredLeft, true);

        const juce::String info = juce::String (bank->numSlices()) + " CHOPS   -   "
            + (bank->flipSeed != 0 ? "FLIPPED  #" + juce::String::toHexString ((int) (bank->flipSeed & 0xffffu)).toUpperCase() : juce::String ("ORIGINAL ORDER"));
        g.setColour (bank->flipSeed != 0 ? col::magenta : col::dim);
        g.setFont (F (11.f, true).withExtraKerningFactor (0.08f));
        g.drawText (info, juce::Rectangle<float> (18.f, 28.f, 420.f, 14.f), juce::Justification::centredLeft, false);

        // Key-Chip
        juce::String key = "KEY  ?";
        if (bank->keyConf > 0.f)
            key = "KEY  " + music::pitchClassName (bank->keyRoot) + (bank->minor ? " MINOR" : " MAJOR")
                  + "   " + juce::String (juce::roundToInt (bank->keyConf * 100.f)) + "%";
        juce::Rectangle<float> chip (b.getRight() - 16.f - 178.f - 12.f - 168.f, 10.f, 168.f, 26.f);
        g.setColour (juce::Colour (0xff0d1120));
        g.fillRoundedRectangle (chip, 13.f);
        g.setColour (col::amber.withAlpha (0.8f));
        g.drawRoundedRectangle (chip, 13.f, 1.f);
        g.setFont (F (12.f, true).withExtraKerningFactor (0.06f));
        g.drawText (key, chip, juce::Justification::centred, false);
    }

    void buildPeaks()
    {
        hiPk.clear(); loPk.clear();
        if (! bank) return;
        const int cols = std::max (1, (int) waveRect().getWidth());
        const auto& buf = *bank->audio;
        const int len = buf.getNumSamples(), ch = buf.getNumChannels();
        hiPk.assign ((size_t) cols, 0.f);
        loPk.assign ((size_t) cols, 0.f);
        float peak = 1.0e-6f;
        for (int c = 0; c < cols; ++c)
        {
            const int s0 = (int) ((juce::int64) len * c / cols), s1 = std::max (s0 + 1, (int) ((juce::int64) len * (c + 1) / cols));
            float hi = 0.f, lo = 0.f;
            for (int k = 0; k < ch; ++k)
            {
                const float* d = buf.getReadPointer (k);
                for (int i = s0; i < s1 && i < len; ++i) { hi = std::max (hi, d[i]); lo = std::min (lo, d[i]); }
            }
            hiPk[(size_t) c] = hi; loPk[(size_t) c] = lo;
            peak = std::max (peak, std::max (hi, -lo));
        }
        for (auto& v : hiPk) v /= peak;
        for (auto& v : loPk) v /= peak;
    }

    void timerCallback() override
    {
        auto nb = proc.getBank();
        if (nb != bank)
        {
            const bool newAudio = ! bank || ! nb || bank->audio != nb->audio;
            bank = nb;
            if (newAudio) buildPeaks();
        }
        repaint();
    }

    FlipGrainProcessor& proc;
    std::shared_ptr<const Bank> bank;
    std::vector<float> hiPk, loPk;
    bool dragOver = false;
    int heldNote = -1;
};

// ============================================================== Keyboard
class KeyboardView : public juce::Component, private juce::Timer
{
public:
    explicit KeyboardView (FlipGrainProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        panelBackground (g, b, 14.f);
        auto bank = proc.getBank();
        const int baseNote = (int) proc.apvts.getRawParameterValue (PID::base)->load();
        const bool chop = (int) proc.apvts.getRawParameterValue (PID::mode)->load() == 0;
        const int n = bank ? bank->numSlices() : 0;

        auto colourFor = [&] (int note, bool& mapped, int& k) -> juce::Colour
        {
            mapped = false; k = note - baseNote;
            if (chop && bank && k >= 0 && k < n)
            {
                mapped = true;
                return sliceColour (bank->order[(size_t) k], n);
            }
            return juce::Colour();
        };

        const float ww = whiteWidth();
        const auto kb = keysArea();
        for (int note = lo; note <= hi; ++note)
        {
            if (isBlack (note)) continue;
            auto r = keyRect (note, kb, ww);
            bool mapped; int k;
            const auto c = colourFor (note, mapped, k);
            const bool on = proc.keyboardState.isNoteOnForChannels (0xffff, note);
            juce::Colour top = mapped ? c.withAlpha (on ? 0.95f : 0.38f) : juce::Colour (on ? 0xff9fb0d8 : 0xff1b2138);
            juce::Colour bot = mapped ? c.withAlpha (on ? 0.7f : 0.14f) : juce::Colour (0xff10152a);
            g.setGradientFill (juce::ColourGradient (top, 0.f, r.getY(), bot, 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r.reduced (1.f, 0.f), 4.f);
            if (on && mapped)
            {
                g.setColour (c.withAlpha (0.25f));
                g.fillRoundedRectangle (r.expanded (2.f), 6.f);
            }
            if (mapped)
            {
                g.setColour (on ? juce::Colour (0xff070914) : c.brighter (0.2f));
                g.setFont (F (10.f, true));
                g.drawText (juce::String (k + 1), r.withTrimmedBottom (4.f), juce::Justification::centredBottom, false);
            }
            else if (note % 12 == 0)
            {
                g.setColour (col::dim);
                g.setFont (F (9.5f));
                g.drawText (music::noteName (note), r.withTrimmedBottom (4.f), juce::Justification::centredBottom, false);
            }
        }
        for (int note = lo; note <= hi; ++note)
        {
            if (! isBlack (note)) continue;
            auto r = keyRect (note, kb, ww);
            bool mapped; int k;
            const auto c = colourFor (note, mapped, k);
            const bool on = proc.keyboardState.isNoteOnForChannels (0xffff, note);
            g.setColour (juce::Colour (0xff05060b));
            g.fillRoundedRectangle (r.expanded (1.f, 0.f), 3.f);
            juce::Colour top = mapped ? c.withAlpha (on ? 1.f : 0.55f) : juce::Colour (on ? 0xff7d8cb8 : 0xff161b2e);
            juce::Colour bot = mapped ? c.darker (0.6f).withAlpha (on ? 0.9f : 0.35f) : juce::Colour (0xff0a0d18);
            g.setGradientFill (juce::ColourGradient (top, 0.f, r.getY(), bot, 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.f);
            if (mapped)
            {
                g.setColour (on ? juce::Colour (0xff070914) : juce::Colours::white.withAlpha (0.9f));
                g.setFont (F (9.f, true));
                g.drawText (juce::String (k + 1), r.withTrimmedBottom (3.f), juce::Justification::centredBottom, false);
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override { press (e); }
    void mouseDrag (const juce::MouseEvent& e) override { press (e); }
    void mouseUp (const juce::MouseEvent&) override { release(); }

private:
    static constexpr int lo = 24, hi = 96;
    FlipGrainProcessor& proc;
    int held = -1;

    static bool isBlack (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    static int whitesBefore (int note)
    {
        int c = 0;
        for (int n = lo; n < note; ++n) if (! isBlack (n)) ++c;
        return c;
    }
    juce::Rectangle<float> keysArea() const { return getLocalBounds().toFloat().reduced (8.f, 8.f); }
    float whiteWidth() const { return keysArea().getWidth() / (float) (whitesBefore (hi) + 1); }
    juce::Rectangle<float> keyRect (int note, juce::Rectangle<float> kb, float ww) const
    {
        if (! isBlack (note))
            return { kb.getX() + ww * (float) whitesBefore (note), kb.getY(), ww, kb.getHeight() };
        const float bw = ww * 0.62f;
        return { kb.getX() + ww * (float) whitesBefore (note) - bw * 0.5f, kb.getY(), bw, kb.getHeight() * 0.6f };
    }
    int noteAt (juce::Point<float> p, float& vel) const
    {
        const auto kb = keysArea();
        const float ww = whiteWidth();
        vel = juce::jlimit (0.3f, 1.f, 0.35f + 0.65f * (p.y - kb.getY()) / kb.getHeight());
        for (int n = lo; n <= hi; ++n) if (isBlack (n) && keyRect (n, kb, ww).contains (p)) return n;
        for (int n = lo; n <= hi; ++n) if (! isBlack (n) && keyRect (n, kb, ww).contains (p)) return n;
        return -1;
    }
    void press (const juce::MouseEvent& e)
    {
        float vel = 0.8f;
        const int n = noteAt (e.position, vel);
        if (n == held) return;
        release();
        if (n >= 0) { held = n; proc.keyboardState.noteOn (1, n, vel); }
    }
    void release()
    {
        if (held >= 0) { proc.keyboardState.noteOff (1, held, 0.f); held = -1; }
    }
    void timerCallback() override { repaint(); }
};
} // namespace fg::ui
