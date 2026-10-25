#pragma once
#include "../Common.h"

namespace fg::ui
{
namespace col
{
    inline const juce::Colour bg0     { 0xff06070c };
    inline const juce::Colour bg1     { 0xff0c0f19 };
    inline const juce::Colour panelTop{ 0xff161b2d };
    inline const juce::Colour panelBot{ 0xff0e1220 };
    inline const juce::Colour border  { 0xff263049 };
    inline const juce::Colour text    { 0xffeaf0ff };
    inline const juce::Colour dim     { 0xff7c88aa };
    inline const juce::Colour faint   { 0xff3a4466 };
    inline const juce::Colour cyan    { 0xff3df2ff };
    inline const juce::Colour violet  { 0xff8a5cff };
    inline const juce::Colour magenta { 0xffff3dcb };
    inline const juce::Colour amber   { 0xffffb23d };
    inline const juce::Colour green   { 0xff5dffa0 };
}

inline juce::Font F (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions ("Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Colour sliceColour (int i, int n)
{
    const float t = n > 1 ? (float) i / (float) (n - 1) : 0.f;
    return juce::Colour::fromHSV (std::fmod (0.50f + 0.5f * t, 1.f), 0.62f, 1.f, 1.f);
}

inline void panelBackground (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setGradientFill (juce::ColourGradient (col::panelTop, r.getX(), r.getY(),
                                             col::panelBot, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.f);
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.drawLine (r.getX() + radius, r.getY() + 1.f, r.getRight() - radius, r.getY() + 1.f);
}
} // namespace fg::ui
