#pragma once
#include "Bank.h"

namespace fg::flipper
{
// Erzeugt aus der analysierten Bank eine "geflippte" Variante (deterministisch aus seed).
std::shared_ptr<Bank> apply (const Bank& base, uint32_t seed, float amount, int keyParam, int scaleParam);

// Erzeugt ein 2-Takte-Pattern (16tel) aus den Chops.
void makePattern (Bank& b, uint32_t seed);

// Schreibt das Pattern als .mid (Noten ab baseNote).
bool writeMidi (const Bank& b, int baseNote, const juce::File& file);
}
