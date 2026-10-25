#pragma once
#include "Bank.h"

namespace fg
{
struct Snapshot
{
    int   mode = 0, engine = 0, base = 36, region = 0;
    bool  loop = false;
    float position = 0.f, scan = 1.f, grainMs = 90.f, density = 28.f, spray = 0.1f;
    float jitterCents = 0.f, spread = 0.5f, revProb = 0.f, shape = 0.8f;
    float retune = 1.f, tune = 0.f;
    int   keyParam = 0, scaleParam = 0;
    double bendRatio = 1.0;
    double hostSR = 44100.0;
};

namespace detail
{
    inline const std::array<float, 513>& halfHann()
    {
        static const auto t = [] {
            std::array<float, 513> a {};
            for (int i = 0; i <= 512; ++i)
                a[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) i / 512.f);
            return a;
        }();
        return t;
    }
    inline float hannAt (float x)   // x: 0..1 -> 0..1 (halbe Hann-Flanke)
    {
        const float p = juce::jlimit (0.f, 1.f, x) * 512.f;
        const int i = (int) p;
        if (i >= 512) return 1.f;
        const auto& t = halfHann();
        return t[(size_t) i] + (t[(size_t) i + 1] - t[(size_t) i]) * (p - (float) i);
    }
    inline float at (const float* d, int n, int k) { return d[juce::jlimit (0, n - 1, k)]; }
    inline float hermite (const float* d, int n, double pos)
    {
        const int i = (int) pos;
        const float f = (float) (pos - (double) i);
        const float y0 = at (d, n, i - 1), y1 = at (d, n, i), y2 = at (d, n, i + 1), y3 = at (d, n, i + 2);
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
}

struct Grain
{
    bool   active = false;
    double pos = 0.0, inc = 1.0;
    int    age = 0, len = 1;
    float  gl = 0.f, gr = 0.f;
    int    segStart = 0, segEnd = 0;
};

class Voice
{
public:
    static constexpr int kMaxGrains = 48;

    bool active = false, released = false;
    int  note = -1, sliceIndex = -1;
    uint64_t tick = 0;

    void prepare (double sr) { adsr.setSampleRate (sr); }

    void start (int n, float velocity, std::shared_ptr<const Bank> b, int sliceIdx, double semis,
                bool sliceReverse, const Snapshot& s, const juce::ADSR::Parameters& ap, uint64_t t)
    {
        note = n; vel = velocity; bank = std::move (b); sliceIndex = sliceIdx; tick = t;
        const int len = bank->audio->getNumSamples();
        if (sliceIdx >= 0)
        {
            rs = bank->slices[(size_t) sliceIdx].start;
            re = bank->slices[(size_t) sliceIdx].end;
        }
        else { rs = 0; re = len; }
        re = std::max (re, rs + 2);
        ratio = std::pow (2.0, semis / 12.0);
        srRatio = bank->sampleRate / s.hostSR;
        baseReverse = sliceReverse;
        engineUsed = s.engine;
        scanPos = 0.0; spawn = 1.0e12; curPh = 0.0; fadeIn = 0;
        ended = false; released = false; active = true;
        for (auto& g : grains) g.active = false;
        const double l = (double) (re - rs);
        classicPos = baseReverse ? (double) (re - 1) - s.position * l : (double) rs + s.position * l;
        adsr.setParameters (ap);
        adsr.reset();
        adsr.noteOn();
    }

    void release()
    {
        if (! released) { released = true; adsr.noteOff(); }
    }

    void render (float* L, float* R, int n, const Snapshot& s, juce::Random& rng)
    {
        if (! active || bank == nullptr) return;
        const auto& buf = *bank->audio;
        const int bl = buf.getNumSamples();
        const float* d0 = buf.getReadPointer (0);
        const float* d1 = buf.getNumChannels() > 1 ? buf.getReadPointer (1) : d0;
        const double l = (double) (re - rs);
        const double interval = s.hostSR / (double) std::max (1.f, s.density);
        const float grainSec = s.grainMs * 0.001f;
        const float norm = 1.f / std::sqrt (std::max (1.f, s.density * grainSec * 0.6f));
        const float fade = juce::jmap (s.shape, 0.05f, 0.5f);

        for (int i = 0; i < n; ++i)
        {
            const float env = adsr.getNextSample();
            if (! adsr.isActive()) { active = false; return; }

            float oL = 0.f, oR = 0.f;

            if (engineUsed == 0)   // ---------------- Granular
            {
                double ph = s.position * l + scanPos;
                if (s.loop)
                {
                    ph = std::fmod (ph, l);
                    if (ph < 0.0) ph += l;
                }
                else if (! ended && (ph >= l || ph < 0.0))
                {
                    ended = true;
                    release();
                }
                curPh = juce::jlimit (0.0, l - 1.0, ph);
                scanPos += (double) s.scan * srRatio;

                if (! ended)
                {
                    spawn += 1.0;
                    if (spawn >= interval)
                    {
                        spawn -= interval;
                        if (spawn > interval) spawn = 0.0;
                        spawnGrain (s, rng, norm);
                    }
                }

                for (auto& g : grains)
                {
                    if (! g.active) continue;
                    const float p = (float) g.age / (float) g.len;
                    float w;
                    if (p < fade)            w = detail::hannAt (p / fade);
                    else if (p > 1.f - fade) w = detail::hannAt ((1.f - p) / fade);
                    else                     w = 1.f;
                    if (g.pos >= (double) g.segStart && g.pos < (double) (g.segEnd - 1))
                    {
                        oL += detail::hermite (d0, bl, g.pos) * w * g.gl;
                        oR += detail::hermite (d1, bl, g.pos) * w * g.gr;
                    }
                    g.pos += g.inc;
                    if (++g.age >= g.len) g.active = false;
                }
            }
            else                   // ---------------- Classic (Varispeed-Sampler)
            {
                const double inc = ratio * srRatio * s.bendRatio * (baseReverse ? -1.0 : 1.0);
                if (! ended)
                {
                    classicPos += inc;
                    if (classicPos >= (double) (re - 1) || classicPos < (double) rs)
                    {
                        if (s.loop) classicPos = inc > 0.0 ? (double) rs : (double) (re - 1);
                        else { ended = true; release(); }
                    }
                }
                if (! ended)
                {
                    const double remain = inc > 0.0 ? (double) (re - 1) - classicPos : classicPos - (double) rs;
                    const double fadeSrc = 96.0 * std::abs (inc);
                    float fo = remain < fadeSrc ? (float) (remain / fadeSrc) : 1.f;
                    const float fi = fadeIn < 48 ? (float) (++fadeIn) / 48.f : 1.f;
                    fo = juce::jlimit (0.f, 1.f, fo);
                    oL = detail::hermite (d0, bl, classicPos) * fi * fo;
                    oR = detail::hermite (d1, bl, classicPos) * fi * fo;
                }
            }

            const float g = env * vel;
            L[i] += oL * g;
            R[i] += oR * g;
        }
    }

    float playheadNorm() const
    {
        if (! active || bank == nullptr) return -1.f;
        const double bl = (double) bank->audio->getNumSamples();
        const double p = engineUsed == 0 ? (double) rs + curPh : classicPos;
        return (float) juce::jlimit (0.0, 1.0, p / bl);
    }

    int collectViz (float* xs, float* as, int maxOut, int offset) const
    {
        if (! active || bank == nullptr) return 0;
        const float bl = (float) bank->audio->getNumSamples();
        int c = 0;
        if (engineUsed == 1)
        {
            if (offset < maxOut && ! ended) { xs[offset] = (float) classicPos / bl; as[offset] = vel; return 1; }
            return 0;
        }
        for (auto& g : grains)
        {
            if (! g.active || offset + c >= maxOut) continue;
            const float p = (float) g.age / (float) g.len;
            xs[offset + c] = (float) g.pos / bl;
            as[offset + c] = std::sin (juce::MathConstants<float>::pi * p) * vel;
            ++c;
        }
        return c;
    }

private:
    void spawnGrain (const Snapshot& s, juce::Random& rng, float norm)
    {
        Grain* slot = nullptr;
        for (auto& g : grains) if (! g.active) { slot = &g; break; }
        if (slot == nullptr) return;

        const double sr = bank->sampleRate;
        const double center = (double) rs + curPh;
        const double jitterSrc = (double) (rng.nextFloat() * 2.f - 1.f) * (double) s.spray * 0.25 * sr;
        const double start = juce::jlimit ((double) rs, (double) (re - 1), center + jitterSrc);

        const double cents = (double) (rng.nextFloat() * 2.f - 1.f) * (double) s.jitterCents;
        const double rat = ratio * s.bendRatio * std::pow (2.0, cents / 1200.0) * srRatio;
        const bool rev = baseReverse != (rng.nextFloat() < s.revProb);

        int len = std::max (64, (int) ((double) s.grainMs * 0.001 * s.hostSR));
        const double avail = rev ? (start - (double) rs) : ((double) (re - 1) - start);
        const int maxLen = (int) (avail / std::max (1.0e-6, rat));
        len = std::min (len, std::max (maxLen, 32));

        const float pan = 0.5f + (rng.nextFloat() - 0.5f) * s.spread;
        const float a = pan * juce::MathConstants<float>::halfPi;
        auto& g = *slot;
        g.active = true;
        g.pos = start;
        g.inc = rev ? -rat : rat;
        g.age = 0;
        g.len = len;
        g.gl = std::cos (a) * 1.4142f * norm;
        g.gr = std::sin (a) * 1.4142f * norm;
        g.segStart = rs;
        g.segEnd = re;
    }

    std::shared_ptr<const Bank> bank;
    juce::ADSR adsr;
    std::array<Grain, kMaxGrains> grains;
    int rs = 0, re = 1, engineUsed = 0, fadeIn = 0;
    double ratio = 1.0, srRatio = 1.0, scanPos = 0.0, spawn = 0.0, curPh = 0.0, classicPos = 0.0;
    float vel = 1.f;
    bool baseReverse = false, ended = false;
};
} // namespace fg
