#pragma once
#include "Common.h"
#include "Music.h"

namespace fg::PID
{
#define FG_ID(n) inline constexpr const char* n = #n;
FG_ID (mode) FG_ID (engine) FG_ID (sliceMode) FG_ID (sens) FG_ID (base) FG_ID (key) FG_ID (scale)
FG_ID (retune) FG_ID (flipAmt) FG_ID (region) FG_ID (loop) FG_ID (position) FG_ID (scan)
FG_ID (grainSize) FG_ID (density) FG_ID (spray) FG_ID (jitter) FG_ID (spread) FG_ID (revProb)
FG_ID (shape) FG_ID (attack) FG_ID (decay) FG_ID (sustain) FG_ID (release) FG_ID (lp) FG_ID (hp)
FG_ID (res) FG_ID (revMix) FG_ID (revSize) FG_ID (tune) FG_ID (gain)
#undef FG_ID
}

namespace fg
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto flt = [&] (const char* id, const char* name, float lo, float hi, float def, float skew,
                    const char* unit, int dec)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.f, skew), def,
            AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (
                [dec] (float v, int) { return String (v, dec); })));
    };
    auto pct = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (0.f, 1.f), def,
            AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (
                [] (float v, int) { return String (roundToInt (v * 100.f)); })));
    };
    auto secs = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.f, 0.35f), def,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return v < 1.f ? String (roundToInt (v * 1000.f)) + " ms" : String (v, 2) + " s"; })));
    };
    auto freq = [&] (const char* id, const char* name, float lo, float hi, float def, float skew)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.f, skew), def,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return v >= 1000.f ? String (v / 1000.f, 2) + " k" : String (roundToInt (v)) + " Hz"; })));
    };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };

    choice (PID::mode,      "Mode",       { "Chop", "Keys" }, 0);
    choice (PID::engine,    "Engine",     { "Granular", "Classic" }, 0);
    choice (PID::sliceMode, "Slice Mode", { "Auto", "4", "8", "16", "32" }, 0);
    pct    (PID::sens,      "Sensitivity", 0.5f);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { PID::base, 1 }, "Root Note", 12, 72, 36,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return music::noteName (v); })));
    choice (PID::key,   "Key",   { "Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    choice (PID::scale, "Scale", { "Auto", "Major", "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian",
                                   "Harm. Minor", "Pent. Major", "Pent. Minor", "Blues" }, 0);
    pct    (PID::retune,  "Retune",      1.0f);
    pct    (PID::flipAmt, "Flip Amount", 0.6f);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { PID::region, 1 }, "Keys Region", 0, 32, 0,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return v == 0 ? String ("All") : String (v); })));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { PID::loop, 1 }, "Loop", false));

    pct (PID::position, "Position", 0.f);
    flt (PID::scan,      "Scan",       -2.f, 2.f, 1.f, 1.f, "x", 2);
    secs(PID::grainSize, "Grain Size", 0.005f, 0.5f, 0.09f);
    flt (PID::density,   "Density",    1.f, 120.f, 28.f, 0.5f, "/s", 0);
    pct (PID::spray,   "Spray",   0.1f);
    flt (PID::jitter,  "Pitch Jitter", 0.f, 100.f, 0.f, 1.f, "ct", 0);
    pct (PID::spread,  "Spread",  0.5f);
    pct (PID::revProb, "Reverse", 0.f);
    pct (PID::shape,   "Shape",   0.8f);

    secs (PID::attack,  "Attack",  0.001f, 5.f, 0.003f);
    secs (PID::decay,   "Decay",   0.001f, 5.f, 0.2f);
    pct  (PID::sustain, "Sustain", 1.f);
    secs (PID::release, "Release", 0.005f, 8.f, 0.12f);

    freq (PID::lp, "Lowpass",  20.f, 20000.f, 20000.f, 0.3f);
    freq (PID::hp, "Highpass", 20.f, 2000.f, 20.f, 0.4f);
    pct  (PID::res, "Resonance", 0.1f);
    pct  (PID::revMix,  "Reverb",      0.12f);
    pct  (PID::revSize, "Reverb Size", 0.5f);
    flt  (PID::tune, "Tune", -24.f, 24.f, 0.f, 1.f, "st", 2);
    flt  (PID::gain, "Gain", -24.f, 12.f, 0.f, 1.f, "dB", 1);

    return { p.begin(), p.end() };
}
} // namespace fg
