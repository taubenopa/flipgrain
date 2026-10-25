#pragma once
#include "Common.h"
#include "Music.h"

namespace fg
{
struct Slice
{
    int   start = 0, end = 0;
    float midi = 60.f;        // erkannte Tonhoehe
    float conf = 0.f;
    bool  pitched = false;
    float flipShift = 0.f;    // vom Flip gewaehlte Verschiebung (Halbtoene)
    bool  reverse = false;
};

struct PatNote { int step = 0, length = 1, key = 0, vel = 100; };

struct Bank
{
    std::shared_ptr<const juce::AudioBuffer<float>> audio;
    double sampleRate = 44100.0;
    std::vector<Slice> slices;
    std::vector<int> order;      // Taste (Offset) -> Slice
    std::vector<int> orderInv;   // Slice -> Taste (Offset)
    int   keyRoot = 0;
    bool  minor = true;
    float keyConf = 0.f;
    juce::String name;
    uint32_t flipSeed = 0;
    float flipAmount = 0.f;
    uint32_t patternSeed = 1;
    std::vector<PatNote> pattern;

    int numSlices() const { return (int) slices.size(); }
};

inline void effectiveKey (const Bank& b, int keyParam, int scaleParam, int& root, const std::vector<int>*& iv)
{
    root = keyParam == 0 ? b.keyRoot : keyParam - 1;
    const int si = scaleParam == 0 ? (b.minor ? 1 : 0) : scaleParam - 1;
    iv = &music::scaleTable()[(size_t) juce::jlimit (0, 9, si)];
}
} // namespace fg
