#pragma once
#include "Common.h"

namespace fg::music
{
inline juce::String noteName (int midi)
{
    static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    const int pc = ((midi % 12) + 12) % 12;
    return juce::String (names[pc]) + juce::String (midi / 12 - 1);
}

inline juce::String pitchClassName (int pc)
{
    static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    return names[((pc % 12) + 12) % 12];
}

// Index in dieser Tabelle = Scale-Parameter - 1 (Parameter 0 = "Auto")
inline const std::array<std::vector<int>, 10>& scaleTable()
{
    static const std::array<std::vector<int>, 10> t {{
        { 0, 2, 4, 5, 7, 9, 11 },   // Major
        { 0, 2, 3, 5, 7, 8, 10 },   // Minor
        { 0, 2, 3, 5, 7, 9, 10 },   // Dorian
        { 0, 1, 3, 5, 7, 8, 10 },   // Phrygian
        { 0, 2, 4, 6, 7, 9, 11 },   // Lydian
        { 0, 2, 4, 5, 7, 9, 10 },   // Mixolydian
        { 0, 2, 3, 5, 7, 8, 11 },   // Harmonic Minor
        { 0, 2, 4, 7, 9 },          // Pentatonic Major
        { 0, 3, 5, 7, 10 },         // Pentatonic Minor
        { 0, 3, 5, 6, 7, 10 }       // Blues
    }};
    return t;
}

inline bool inScale (int midi, int root, const std::vector<int>& iv)
{
    const int pc = (((midi - root) % 12) + 12) % 12;
    return std::find (iv.begin(), iv.end(), pc) != iv.end();
}

// naechster Skalenton (ganzzahlig)
inline int snap (float midi, int root, const std::vector<int>& iv)
{
    const int c = (int) std::lround (midi);
    int best = c;
    float bestD = 1.0e9f;
    for (int n = c - 7; n <= c + 7; ++n)
        if (inScale (n, root, iv))
        {
            const float d = std::abs ((float) n - midi);
            if (d < bestD - 1.0e-6f) { bestD = d; best = n; }
        }
    return best;
}

// 'steps' Skalenstufen weiter (positiv/negativ), Start muss in der Skala liegen
inline int stepScale (int midiInScale, int steps, int root, const std::vector<int>& iv)
{
    int n = midiInScale;
    const int dir = steps >= 0 ? 1 : -1;
    int count = std::abs (steps);
    while (count > 0)
    {
        n += dir;
        if (inScale (n, root, iv)) --count;
    }
    return n;
}
} // namespace fg::music
