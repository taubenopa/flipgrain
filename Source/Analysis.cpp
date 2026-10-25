#include "Analysis.h"

namespace fg::analysis
{
namespace
{
    struct Onset { int pos; float strength; };

    std::vector<float> mixToMono (const juce::AudioBuffer<float>& b)
    {
        const int n = b.getNumSamples(), ch = b.getNumChannels();
        std::vector<float> m ((size_t) n, 0.f);
        for (int c = 0; c < ch; ++c)
        {
            auto* d = b.getReadPointer (c);
            for (int i = 0; i < n; ++i) m[(size_t) i] += d[i];
        }
        if (ch > 1) for (auto& v : m) v /= (float) ch;
        return m;
    }

    // Feintuning: steilster Energieanstieg rund um die Schaetzung
    int refineOnset (const std::vector<float>& mono, int guess)
    {
        const int blk = 32;
        const int a = std::max (blk, guess - 1536);
        const int b = std::min ((int) mono.size() - blk, guess + 512);
        if (b - a < blk * 4) return std::max (0, guess);
        std::vector<float> r;
        for (int i = a; i < b; i += blk)
        {
            float s = 0.f;
            for (int j = 0; j < blk; ++j) s += mono[(size_t) (i + j)] * mono[(size_t) (i + j)];
            r.push_back (std::sqrt (s / (float) blk));
        }
        int bestK = 1; float bestD = -1.f;
        for (size_t k = 1; k < r.size(); ++k)
        {
            const float d = r[k] - r[k - 1];
            if (d > bestD) { bestD = d; bestK = (int) k; }
        }
        return std::max (0, a + (bestK - 1) * blk);
    }

    std::vector<Onset> findOnsets (const std::vector<float>& mono, double sr, float sens)
    {
        constexpr int order = 10, N = 1 << order, hop = 256;
        const int total = (int) mono.size();
        if (total < N * 4) return {};

        juce::dsp::FFT fft (order);
        std::vector<float> win ((size_t) N);
        for (int i = 0; i < N; ++i)
            win[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) i / (float) (N - 1));

        const int frames = (total - N) / hop + 1;
        std::vector<float> data ((size_t) N * 2), prev ((size_t) N / 2 + 1, 0.f), flux ((size_t) frames, 0.f);

        for (int f = 0; f < frames; ++f)
        {
            std::fill (data.begin(), data.end(), 0.f);
            const float* src = mono.data() + (size_t) f * hop;
            for (int i = 0; i < N; ++i) data[(size_t) i] = src[i] * win[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (data.data());
            float sum = 0.f;
            for (int k = 2; k <= N / 2; ++k)
            {
                const float m = std::log1p (data[(size_t) k] * 10.f);
                const float d = m - prev[(size_t) k];
                if (d > 0.f && f > 0) sum += d;
                prev[(size_t) k] = m;
            }
            flux[(size_t) f] = sum;
        }

        std::vector<double> cs ((size_t) frames + 1, 0.0);
        for (int i = 0; i < frames; ++i) cs[(size_t) i + 1] = cs[(size_t) i] + flux[(size_t) i];
        const float gmax = *std::max_element (flux.begin(), flux.end());
        if (gmax <= 0.f) return {};

        const float mult  = 1.15f + 2.2f * (1.f - sens);
        const float delta = gmax * (0.01f + 0.06f * (1.f - sens));
        constexpr int W = 16, P = 4;

        std::vector<Onset> out;
        for (int f = P; f < frames - P; ++f)
        {
            const int a = std::max (0, f - W), b = std::min (frames, f + W + 1);
            const float mean = (float) ((cs[(size_t) b] - cs[(size_t) a]) / (double) (b - a));
            const float v = flux[(size_t) f];
            if (v <= mean * mult + delta) continue;
            bool isMax = true;
            for (int k = f - P; k <= f + P && isMax; ++k)
                if (flux[(size_t) k] > v) isMax = false;
            if (! isMax) continue;
            const int guess = f * hop + N / 2 - 2 * hop;
            out.push_back ({ refineOnset (mono, guess), v });
        }

        std::sort (out.begin(), out.end(), [] (const Onset& x, const Onset& y) { return x.pos < y.pos; });
        const int minGap = (int) (0.05 * sr);
        std::vector<Onset> clean;
        for (auto& o : out)
        {
            if (! clean.empty() && o.pos - clean.back().pos < minGap)
            {
                if (o.strength > clean.back().strength) clean.back() = o;
            }
            else clean.push_back (o);
        }
        return clean;
    }

    // YIN Pitch-Erkennung
    bool yin (const float* x, int n, double sr, float& midiOut, float& confOut)
    {
        const int tauMax = (int) (sr / 60.0);
        const int tauMin = std::max (2, (int) (sr / 1500.0));
        int W = n - tauMax;
        if (W < 512) return false;
        W = std::min (W, 2048);

        double energy = 0.0;
        for (int j = 0; j < W; ++j) energy += (double) x[j] * x[j];
        if (std::sqrt (energy / W) < 0.004) return false;

        std::vector<float> cm ((size_t) tauMax + 1, 1.f);
        double run = 0.0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double sum = 0.0;
            for (int j = 0; j < W; ++j) { const double d = (double) x[j] - x[j + tau]; sum += d * d; }
            run += sum;
            cm[(size_t) tau] = run > 0.0 ? (float) (sum * tau / run) : 1.f;
        }

        int tau = -1;
        for (int t = tauMin; t < tauMax; ++t)
            if (cm[(size_t) t] < 0.15f)
            {
                while (t + 1 < tauMax && cm[(size_t) t + 1] < cm[(size_t) t]) ++t;
                tau = t; break;
            }
        if (tau < 0)
        {
            float best = 1.f;
            for (int t = tauMin; t < tauMax; ++t)
                if (cm[(size_t) t] < best) { best = cm[(size_t) t]; tau = t; }
            if (tau < 0 || best > 0.30f) return false;
        }

        const float x0 = cm[(size_t) tau - 1], x1 = cm[(size_t) tau], x2 = cm[(size_t) tau + 1];
        const float den = x0 - 2.f * x1 + x2;
        const float shift = std::abs (den) > 1.0e-9f ? juce::jlimit (-1.f, 1.f, 0.5f * (x0 - x2) / den) : 0.f;
        const double f0 = sr / ((double) tau + shift);
        if (f0 < 60.0 || f0 > 1500.0) return false;
        midiOut = (float) (69.0 + 12.0 * std::log2 (f0 / 440.0));
        confOut = 1.f - x1;
        return true;
    }

    void detectSlicePitch (const std::vector<float>& mono, Slice& s, double sr)
    {
        const int skip = std::min ((int) ((s.end - s.start) * 0.1), (int) (0.02 * sr));
        const int s0 = s.start + skip;
        const int tauMax = (int) (sr / 60.0);
        const int need = tauMax + 2048;
        float bestConf = 0.f, bestMidi = 60.f;
        for (float frac : { 0.f, 0.3f, 0.6f })
        {
            const int off = s0 + (int) ((float) (s.end - s0) * frac * 0.5f);
            const int avail = std::min (s.end, (int) mono.size()) - off;
            if (avail < tauMax + 512) continue;
            float m = 0.f, c = 0.f;
            if (yin (mono.data() + off, std::min (avail, need), sr, m, c) && c > bestConf)
            { bestConf = c; bestMidi = m; }
        }
        s.conf = bestConf;
        s.pitched = bestConf >= 0.80f;
        if (s.pitched) s.midi = bestMidi;
    }

    // Krumhansl-Schmuckler Tonart-Erkennung
    void detectKey (Bank& b)
    {
        static const float maj[12] = { 6.35f,2.23f,3.48f,2.33f,4.38f,4.09f,2.52f,5.19f,2.39f,3.66f,2.29f,2.88f };
        static const float mnr[12] = { 6.33f,2.68f,3.52f,5.38f,2.60f,3.53f,2.54f,4.75f,3.98f,2.69f,3.34f,3.17f };
        float hist[12] = {};
        float total = 0.f;
        for (auto& s : b.slices)
            if (s.pitched)
            {
                const int pc = ((int) std::lround (s.midi) % 12 + 12) % 12;
                const float w = s.conf * (float) (s.end - s.start);
                hist[pc] += w; total += w;
            }
        if (total <= 0.f) { b.keyRoot = 0; b.minor = true; b.keyConf = 0.f; return; }

        auto corr = [&] (const float* prof, int rot)
        {
            float mh = 0, mp = 0;
            for (int i = 0; i < 12; ++i) { mh += hist[i]; mp += prof[i]; }
            mh /= 12.f; mp /= 12.f;
            float num = 0, dh = 0, dp = 0;
            for (int i = 0; i < 12; ++i)
            {
                const float h = hist[(i + rot) % 12] - mh, p = prof[i] - mp;
                num += h * p; dh += h * h; dp += p * p;
            }
            return (dh > 0 && dp > 0) ? num / std::sqrt (dh * dp) : 0.f;
        };

        float best = -2.f; int bestRoot = 0; bool bestMinor = true;
        for (int r = 0; r < 12; ++r)
        {
            const float cM = corr (maj, r), cm = corr (mnr, r);
            if (cM > best) { best = cM; bestRoot = r; bestMinor = false; }
            if (cm > best) { best = cm; bestRoot = r; bestMinor = true; }
        }
        b.keyRoot = bestRoot; b.minor = bestMinor; b.keyConf = juce::jlimit (0.f, 1.f, best);
    }
}

std::shared_ptr<Bank> analyse (std::shared_ptr<const juce::AudioBuffer<float>> audio, double sr,
                               const juce::String& name, int sliceModeIdx, float sens)
{
    auto bank = std::make_shared<Bank>();
    bank->audio = audio;
    bank->sampleRate = sr;
    bank->name = name;

    const int len = audio->getNumSamples();
    const auto mono = mixToMono (*audio);
    std::vector<int> starts;

    auto grid = [&] (int n)
    {
        starts.clear();
        for (int i = 0; i < n; ++i) starts.push_back ((int) ((juce::int64) len * i / n));
    };

    if (sliceModeIdx > 0)
    {
        static const int counts[] = { 4, 8, 16, 32 };
        grid (counts[juce::jlimit (0, 3, sliceModeIdx - 1)]);
    }
    else
    {
        auto on = findOnsets (mono, sr, sens);
        const int minLen = (int) (0.04 * sr);
        on.erase (std::remove_if (on.begin(), on.end(), [&] (const Onset& o) { return o.pos < minLen; }), on.end());
        if (on.size() > 31)
        {
            std::sort (on.begin(), on.end(), [] (const Onset& a, const Onset& b) { return a.strength > b.strength; });
            on.resize (31);
            std::sort (on.begin(), on.end(), [] (const Onset& a, const Onset& b) { return a.pos < b.pos; });
        }
        starts.push_back (0);
        for (auto& o : on)
            if (o.pos - starts.back() >= minLen && len - o.pos >= minLen) starts.push_back (o.pos);
        if (starts.size() < 2) grid (8);
    }

    for (size_t i = 0; i < starts.size(); ++i)
    {
        Slice s;
        s.start = starts[i];
        s.end = i + 1 < starts.size() ? starts[i + 1] : len;
        bank->slices.push_back (s);
    }
    for (auto& s : bank->slices) detectSlicePitch (mono, s, sr);
    detectKey (*bank);

    const int n = bank->numSlices();
    bank->order.resize ((size_t) n);
    bank->orderInv.resize ((size_t) n);
    for (int i = 0; i < n; ++i) { bank->order[(size_t) i] = i; bank->orderInv[(size_t) i] = i; }
    return bank;
}
} // namespace fg::analysis
