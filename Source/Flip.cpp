#include "Flip.h"

namespace fg::flipper
{
std::shared_ptr<Bank> apply (const Bank& base, uint32_t seed, float amount, int keyParam, int scaleParam)
{
    auto nb = std::make_shared<Bank> (base);
    const int n = nb->numSlices();
    for (auto& s : nb->slices) { s.flipShift = 0.f; s.reverse = false; }
    for (int i = 0; i < n; ++i) { nb->order[(size_t) i] = i; nb->orderInv[(size_t) i] = i; }
    nb->flipSeed = seed;
    nb->flipAmount = amount;
    if (seed == 0 || n == 0) return nb;

    std::mt19937 rng (seed);
    auto u = [&] { return std::uniform_real_distribution<float> (0.f, 1.f) (rng); };

    int root = 0;
    const std::vector<int>* iv = nullptr;
    effectiveKey (*nb, keyParam, scaleParam, root, iv);

    static const int stepChoices[7] = { -3, -2, -1, 1, 2, 3, 4 };
    std::discrete_distribution<int> stepDist { 1, 2, 3, 3, 2, 1, 1 };

    for (auto& s : nb->slices)
    {
        if (s.pitched)
        {
            const int cur = music::snap (s.midi, root, *iv);
            float target = (float) cur;
            if (u() < amount * 0.85f)
            {
                const int steps = stepChoices[stepDist (rng)];
                int t = music::stepScale (cur, steps, root, *iv);
                if (t - s.midi > 12.f) t -= 12;
                if (t - s.midi < -12.f) t += 12;
                target = (float) t;
            }
            // Verschiebung relativ zum (auf die Skala gerundeten) Originalton.
            // Der Retune-Anteil wird live im Processor berechnet.
            float shift = target - (float) cur;
            if (u() < amount * 0.15f) shift += (u() < 0.5f ? -12.f : 12.f);
            s.flipShift = juce::jlimit (-19.f, 19.f, shift);
        }
        s.reverse = u() < amount * 0.25f;
    }

    const int swaps = (int) (amount * (float) n * 1.2f);
    for (int k = 0; k < swaps; ++k)
    {
        const int i = (int) (rng() % (uint32_t) n), j = (int) (rng() % (uint32_t) n);
        std::swap (nb->order[(size_t) i], nb->order[(size_t) j]);
    }
    for (int k = 0; k < n; ++k) nb->orderInv[(size_t) nb->order[(size_t) k]] = k;

    makePattern (*nb, seed * 2654435761u + 7u);
    return nb;
}

void makePattern (Bank& b, uint32_t seed)
{
    b.pattern.clear();
    b.patternSeed = seed;
    const int n = b.numSlices();
    if (n <= 0) return;

    std::mt19937 rng (seed * 747796405u + 2891336453u);
    auto u = [&] { return std::uniform_real_distribution<float> (0.f, 1.f) (rng); };

    static const float prob[16] = { 1.f, .15f, .35f, .6f, .8f, .15f, .4f, .5f,
                                    .9f, .2f, .45f, .55f, .75f, .2f, .5f, .4f };
    std::vector<char> A (16, 0), B;
    for (int s = 0; s < 16; ++s) A[(size_t) s] = u() < prob[s];
    A[0] = 1;
    B = A;
    for (int s = 1; s < 16; ++s) if (u() < 0.3f) B[(size_t) s] = ! B[(size_t) s];
    B[0] = 1;

    std::vector<int> steps;
    for (int s = 0; s < 16; ++s) if (A[(size_t) s]) steps.push_back (s);
    for (int s = 0; s < 16; ++s) if (B[(size_t) s]) steps.push_back (16 + s);

    int cur = (int) (rng() % (uint32_t) n);
    for (size_t i = 0; i < steps.size(); ++i)
    {
        const int next = i + 1 < steps.size() ? steps[i + 1] : 32;
        const int gap = next - steps[i];
        PatNote pn;
        pn.step = steps[i];
        pn.length = std::max (1, std::min (gap, 1 + (int) (rng() % 3u)));
        const float r = u();
        if (r < 0.55f)      cur = (cur + 1) % n;
        else if (r < 0.75f) { /* bleibt */ }
        else if (r < 0.90f) cur = (int) (rng() % (uint32_t) n);
        else                cur = (cur - 1 + n) % n;
        pn.key = cur;
        pn.vel = 78 + (int) (rng() % 43u);
        b.pattern.push_back (pn);
    }
}

bool writeMidi (const Bank& b, int baseNote, const juce::File& file)
{
    if (b.pattern.empty()) return false;
    const int tpq = 960, stepTicks = tpq / 4;
    juce::MidiMessageSequence seq;
    for (auto& pn : b.pattern)
    {
        const int note = juce::jlimit (0, 127, baseNote + pn.key);
        seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, pn.vel)),
                      (double) (pn.step * stepTicks));
        seq.addEvent (juce::MidiMessage::noteOff (1, note),
                      (double) ((pn.step + pn.length) * stepTicks - 6));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    file.deleteFile();
    juce::FileOutputStream os (file);
    if (! os.openedOk()) return false;
    return mf.writeTo (os);
}
} // namespace fg::flipper
