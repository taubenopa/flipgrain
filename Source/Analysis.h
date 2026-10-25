#pragma once
#include "Bank.h"

namespace fg::analysis
{
// sliceModeIdx: 0 = Auto (Transienten), 1..4 = 4/8/16/32 gleichmaessig
std::shared_ptr<Bank> analyse (std::shared_ptr<const juce::AudioBuffer<float>> audio, double sr,
                               const juce::String& name, int sliceModeIdx, float sensitivity);
}
