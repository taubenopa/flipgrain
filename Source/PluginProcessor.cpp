#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Analysis.h"
#include "Flip.h"

using namespace fg;

namespace
{
std::shared_ptr<juce::AudioBuffer<float>> readAll (juce::AudioFormatReader& r, double& sr)
{
    const juce::int64 maxLen = (juce::int64) (r.sampleRate * 120.0);   // max. 2 Minuten
    const juce::int64 len = juce::jmin (r.lengthInSamples, maxLen);
    if (len < 256 || r.numChannels < 1) return nullptr;
    auto buf = std::make_shared<juce::AudioBuffer<float>> ((int) juce::jmin (2u, r.numChannels), (int) len);
    if (! r.read (buf.get(), 0, (int) len, 0, true, true)) return nullptr;
    sr = r.sampleRate;
    return buf;
}
}

FlipGrainProcessor::FlipGrainProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", fg::createLayout()),
      pool (1)
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p = { get (PID::mode), get (PID::engine), get (PID::sliceMode), get (PID::sens), get (PID::base), get (PID::key),
          get (PID::scale), get (PID::retune), get (PID::flipAmt), get (PID::region), get (PID::loop),
          get (PID::position), get (PID::scan), get (PID::grainSize), get (PID::density), get (PID::spray),
          get (PID::jitter), get (PID::spread), get (PID::revProb), get (PID::shape), get (PID::attack),
          get (PID::decay), get (PID::sustain), get (PID::release), get (PID::lp), get (PID::hp), get (PID::res),
          get (PID::revMix), get (PID::revSize), get (PID::tune), get (PID::gain) };

    for (auto& a : playheads) a.store (-1.f);
    apvts.addParameterListener (PID::sliceMode, this);
    apvts.addParameterListener (PID::sens, this);
    startTimer (100);
}

FlipGrainProcessor::~FlipGrainProcessor()
{
    stopTimer();
    cancelPendingUpdate();
    apvts.removeParameterListener (PID::sliceMode, this);
    apvts.removeParameterListener (PID::sens, this);
    pool.removeAllJobs (true, 10000);
}

// ------------------------------------------------------------------ Audio
void FlipGrainProcessor::prepareToPlay (double sr, int blockSize)
{
    currentSR = sr;
    for (auto& v : voices) { v.prepare (sr); v.active = false; }
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };
    lpFilter.prepare (spec); hpFilter.prepare (spec);
    lpFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    hpFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    reverb.setSampleRate (sr);
    reverb.reset();
    gainSm.reset (sr, 0.02);
    gainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.gain->load()));
    monoScratch.assign ((size_t) blockSize + 16, 0.f);
}

bool FlipGrainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

fg::Snapshot FlipGrainProcessor::makeSnapshot() const
{
    Snapshot s;
    s.mode = (int) p.mode->load();
    s.engine = (int) p.engine->load();
    s.base = (int) p.base->load();
    s.region = (int) p.region->load();
    s.loop = p.loop->load() > 0.5f;
    s.position = p.position->load();
    s.scan = p.scan->load();
    s.grainMs = p.grainSize->load() * 1000.f;
    s.density = p.density->load();
    s.spray = p.spray->load();
    s.jitterCents = p.jitter->load();
    s.spread = p.spread->load();
    s.revProb = p.revProb->load();
    s.shape = p.shape->load();
    s.retune = p.retune->load();
    s.tune = p.tune->load();
    s.keyParam = (int) p.key->load();
    s.scaleParam = (int) p.scale->load();
    s.bendRatio = std::pow (2.0, bendSemis / 12.0);
    s.hostSR = currentSR;
    return s;
}

float FlipGrainProcessor::getSliceSemis (const Bank& b, int idx) const
{
    if (idx < 0 || idx >= b.numSlices()) return 0.f;
    const auto& s = b.slices[(size_t) idx];
    if (! s.pitched) return 0.f;
    int root = 0;
    const std::vector<int>* iv = nullptr;
    effectiveKey (b, (int) p.key->load(), (int) p.scale->load(), root, iv);
    const float base = s.midi + s.flipShift;
    const int snapped = music::snap (base, root, *iv);
    return s.flipShift + ((float) snapped - base) * p.retune->load();
}

Voice* FlipGrainProcessor::allocateVoice()
{
    for (auto& v : voices) if (! v.active) return &v;
    Voice* best = nullptr;
    for (auto& v : voices)                         // zuerst: aelteste Release-Stimme
        if (v.released && (best == nullptr || v.tick < best->tick)) best = &v;
    if (best) return best;
    for (auto& v : voices) if (best == nullptr || v.tick < best->tick) best = &v;
    return best;
}

void FlipGrainProcessor::noteOn (int note, float vel, const Snapshot& snap, const juce::ADSR::Parameters& ap)
{
    if (audioBank == nullptr || audioBank->numSlices() == 0) return;
    const Bank& b = *audioBank;
    int sliceIdx = -1;
    double semis = 0.0;
    bool rev = false;

    if (snap.mode == 0)   // Chop
    {
        const int k = note - snap.base;
        if (k < 0 || k >= b.numSlices()) return;
        sliceIdx = b.order[(size_t) k];
        semis = (double) getSliceSemis (b, sliceIdx) + (double) snap.tune;
        rev = b.slices[(size_t) sliceIdx].reverse;
    }
    else                  // Keys
    {
        double rootMidi = 60.0;
        if (snap.region > 0 && snap.region <= b.numSlices())
        {
            sliceIdx = snap.region - 1;
            const auto& s = b.slices[(size_t) sliceIdx];
            if (s.pitched) rootMidi = (double) s.midi + (double) getSliceSemis (b, sliceIdx);
            rev = s.reverse;
        }
        semis = (double) note - rootMidi + (double) snap.tune;
    }

    for (auto& v : voices)    // gleiche Taste -> alte Stimme ausklingen lassen
        if (v.active && v.note == note && ! v.released) v.release();

    if (auto* v = allocateVoice())
        v->start (note, std::pow (vel, 0.75f), audioBank, sliceIdx, semis, rev, snap, ap, ++tickCounter);
}

void FlipGrainProcessor::handleMidi (const juce::MidiMessage& m, Snapshot& snap, const juce::ADSR::Parameters& ap)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity(), snap, ap);
    else if (m.isNoteOff())
    {
        for (auto& v : voices) if (v.active && v.note == m.getNoteNumber() && ! v.released) v.release();
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (auto& v : voices) if (v.active) v.release();
    }
    else if (m.isPitchWheel())
    {
        bendSemis = ((double) m.getPitchWheelValue() - 8192.0) / 8192.0 * 2.0;
        snap.bendRatio = std::pow (2.0, bendSemis / 12.0);
    }
}

void FlipGrainProcessor::renderVoices (float* L, float* R, int n, const Snapshot& s)
{
    for (auto& v : voices) if (v.active) v.render (L, R, n, s, rng);
}

void FlipGrainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    {
        juce::SpinLock::ScopedTryLockType l (bankLock);
        if (l.isLocked() && audioBankVersion != bankVersion.load())
        {
            audioBank = current;
            audioBankVersion = bankVersion.load();
            for (auto& v : voices) v.active = false;
        }
    }

    Snapshot snap = makeSnapshot();
    juce::ADSR::Parameters ap { p.attack->load(), p.decay->load(), p.sustain->load(), p.release->load() };

    const bool mono = buffer.getNumChannels() < 2;
    float* L = buffer.getWritePointer (0);
    float* R = mono ? monoScratch.data() : buffer.getWritePointer (1);
    if (mono) std::fill (monoScratch.begin(), monoScratch.begin() + n, 0.f);
    if (mono && (int) monoScratch.size() < n) return;

    int pos = 0;
    for (const auto meta : midi)
    {
        const int ev = juce::jlimit (0, n, meta.samplePosition);
        if (ev > pos) { renderVoices (L + pos, R + pos, ev - pos, snap); pos = ev; }
        handleMidi (meta.getMessage(), snap, ap);
    }
    if (pos < n) renderVoices (L + pos, R + pos, n - pos, snap);

    if (mono)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
    else
    {
        const float hpF = p.hp->load(), lpF = p.lp->load();
        juce::dsp::AudioBlock<float> blk (buffer);
        auto stereo = blk.getSubsetChannelBlock (0, 2);
        juce::dsp::ProcessContextReplacing<float> ctx (stereo);
        if (hpF > 21.f) { hpFilter.setCutoffFrequency (hpF); hpFilter.process (ctx); }
        if (lpF < 19900.f)
        {
            lpFilter.setCutoffFrequency (lpF);
            lpFilter.setResonance (0.707f * std::pow (11.3f, p.res->load()));
            lpFilter.process (ctx);
        }

        const float mix = p.revMix->load();
        if (mix > 0.003f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.3f + 0.69f * p.revSize->load();
            rp.damping = 0.45f; rp.width = 1.f;
            rp.wetLevel = mix * 0.55f;
            rp.dryLevel = 1.f - mix * 0.35f;
            rp.freezeMode = 0.f;
            reverb.setParameters (rp);
            reverb.processStereo (L, R, n);
        }
    }

    gainSm.setTargetValue (juce::Decibels::decibelsToGain (p.gain->load()));
    float peakL = 0.f, peakR = 0.f;
    auto softClip = [] (float x)
    {
        const float a = std::abs (x);
        if (a <= 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f);
        return x < 0.f ? -y : y;
    };
    for (int i = 0; i < n; ++i)
    {
        const float g = gainSm.getNextValue();
        L[i] = softClip (L[i] * g);
        peakL = std::max (peakL, std::abs (L[i]));
        if (! mono)
        {
            R[i] = softClip (R[i] * g);
            peakR = std::max (peakR, std::abs (R[i]));
        }
    }
    if (mono) peakR = peakL;
    meterL.store (std::max (meterL.load(), peakL));
    meterR.store (std::max (meterR.load(), peakR));

    updateViz();
}

void FlipGrainProcessor::updateViz()
{
    std::array<float, kViz> xs {}, as {};
    int count = 0;
    uint64_t mask = 0;
    for (size_t i = 0; i < voices.size(); ++i)
    {
        const auto& v = voices[i];
        playheads[i].store (v.active ? v.playheadNorm() : -1.f, std::memory_order_relaxed);
        if (! v.active) continue;
        if (v.sliceIndex >= 0 && v.sliceIndex < 64) mask |= (uint64_t) 1 << v.sliceIndex;
        count += v.collectViz (xs.data(), as.data(), kViz, count);
    }
    for (int i = 0; i < kViz; ++i)
    {
        vizX[(size_t) i].store (i < count ? xs[(size_t) i] : 0.f, std::memory_order_relaxed);
        vizA[(size_t) i].store (i < count ? as[(size_t) i] : 0.f, std::memory_order_relaxed);
    }
    activeSliceMask.store (mask, std::memory_order_relaxed);
}

// ------------------------------------------------------------------ Bank / Worker
std::shared_ptr<const Bank> FlipGrainProcessor::getBank() const
{
    const juce::SpinLock::ScopedLockType l (bankLock);
    return current;
}

void FlipGrainProcessor::setBank (std::shared_ptr<const Bank> b)
{
    const juce::SpinLock::ScopedLockType l (bankLock);
    if (current) graveyard.push_back (current);
    current = std::move (b);
    ++bankVersion;
}

void FlipGrainProcessor::setStatus (const juce::String& s)
{
    const juce::ScopedLock sl (dataLock);
    statusText = s;
}

juce::String FlipGrainProcessor::getStatus() const
{
    const juce::ScopedLock sl (dataLock);
    return statusText;
}

void FlipGrainProcessor::timerCallback()
{
    if (rebuildCountdown.load() > 0 && --rebuildCountdown == 0)
        pool.addJob ([this] { rebuild(); });

    if (++housekeeping >= 5)
    {
        housekeeping = 0;
        std::vector<std::shared_ptr<const Bank>> dead;
        {
            const juce::SpinLock::ScopedLockType l (bankLock);
            for (auto it = graveyard.begin(); it != graveyard.end();)
            {
                if (it->use_count() == 1) { dead.push_back (std::move (*it)); it = graveyard.erase (it); }
                else ++it;
            }
        }
    }
}

void FlipGrainProcessor::parameterChanged (const juce::String& id, float)
{
    if (id == PID::sliceMode || id == PID::sens) triggerAsyncUpdate();
}

void FlipGrainProcessor::handleAsyncUpdate() { rebuildCountdown = 3; }

void FlipGrainProcessor::loadSampleFile (const juce::File& file)
{
    loading = true;
    setStatus ("Loading " + file.getFileName() + " ...");
    pool.addJob ([this, file]
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        double sr = 44100.0;
        std::shared_ptr<juce::AudioBuffer<float>> buf;
        if (r) buf = readAll (*r, sr);
        if (! buf)
        {
            loading = false;
            setStatus ("Could not read " + file.getFileName());
            return;
        }
        installAudio (buf, sr, file.getFileNameWithoutExtension(), file.getFullPathName(), true);
        setStatus ({});
    });
}

void FlipGrainProcessor::installAudio (std::shared_ptr<juce::AudioBuffer<float>> buf, double sr,
                                       const juce::String& name, const juce::String& path, bool resetFlipState)
{
    const float peak = buf->getMagnitude (0, buf->getNumSamples());
    if (peak > 1.0e-5f) buf->applyGain (0.89f / peak);

    juce::MemoryBlock flacBlock;
    {
        juce::FlacAudioFormat flac;
        std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (
            new juce::MemoryOutputStream (flacBlock, false), sr, (unsigned) buf->getNumChannels(), 24,
            juce::StringPairArray(), 5));
        if (w) w->writeFromAudioSampleBuffer (*buf, 0, buf->getNumSamples());
    }
    {
        const juce::ScopedLock sl (dataLock);
        audioSource = buf;
        sourceRate = sr;
        sampleName = name;
        samplePath = path;
        flacCache = std::move (flacBlock);
    }
    if (resetFlipState) flipSeed = 0;
    rebuild();
}

void FlipGrainProcessor::rebuild()
{
    std::shared_ptr<const juce::AudioBuffer<float>> src;
    double sr = 44100.0;
    juce::String nm;
    {
        const juce::ScopedLock sl (dataLock);
        src = audioSource; sr = sourceRate; nm = sampleName;
    }
    if (! src) { loading = false; return; }
    loading = true;

    auto base = analysis::analyse (src, sr, nm, (int) p.sliceMode->load(), p.sens->load());
    flipper::makePattern (*base, 1);
    baseBank = base;

    std::shared_ptr<const Bank> out = base;
    if (flipSeed.load() != 0)
        out = flipper::apply (*base, flipSeed.load(), flipAmtUsed.load(), flipKeyUsed.load(), flipScaleUsed.load());
    setBank (out);
    loading = false;
}

void FlipGrainProcessor::flipSample()
{
    pool.addJob ([this]
    {
        if (! baseBank) return;
        uint32_t seed = (uint32_t) juce::Random::getSystemRandom().nextInt() | 1u;
        flipSeed = seed;
        flipAmtUsed = p.flipAmt->load();
        flipKeyUsed = (int) p.key->load();
        flipScaleUsed = (int) p.scale->load();
        setBank (flipper::apply (*baseBank, seed, flipAmtUsed.load(), flipKeyUsed.load(), flipScaleUsed.load()));
    });
}

void FlipGrainProcessor::resetFlip()
{
    pool.addJob ([this]
    {
        if (! baseBank) return;
        flipSeed = 0;
        setBank (baseBank);
    });
}

bool FlipGrainProcessor::exportPatternTo (const juce::File& f)
{
    auto b = getBank();
    return b && flipper::writeMidi (*b, (int) p.base->load(), f);
}

// ------------------------------------------------------------------ State
void FlipGrainProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    auto data = state.getOrCreateChildWithName ("FGDATA", nullptr);
    {
        const juce::ScopedLock sl (dataLock);
        data.setProperty ("name", sampleName, nullptr);
        data.setProperty ("path", samplePath, nullptr);
        data.setProperty ("flac", flacCache.getSize() > 0 ? flacCache.toBase64Encoding() : juce::String(), nullptr);
    }
    data.setProperty ("flipSeed", (juce::int64) flipSeed.load(), nullptr);
    data.setProperty ("flipAmt", (double) flipAmtUsed.load(), nullptr);
    data.setProperty ("flipKey", flipKeyUsed.load(), nullptr);
    data.setProperty ("flipScale", flipScaleUsed.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void FlipGrainProcessor::setStateInformation (const void* d, int size)
{
    auto xml = getXmlFromBinary (d, size);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;

    auto tree = juce::ValueTree::fromXml (*xml);
    auto data = tree.getChildWithName ("FGDATA");
    apvts.replaceState (tree);
    rebuildCountdown = 0;
    if (! data.isValid()) return;

    flipSeed = (uint32_t) (juce::int64) data.getProperty ("flipSeed", 0);
    flipAmtUsed = (float) (double) data.getProperty ("flipAmt", 0.6);
    flipKeyUsed = (int) data.getProperty ("flipKey", 0);
    flipScaleUsed = (int) data.getProperty ("flipScale", 0);

    const juce::String b64 = data.getProperty ("flac").toString();
    const juce::String path = data.getProperty ("path").toString();
    const juce::String name = data.getProperty ("name").toString();
    if (b64.isEmpty() && path.isEmpty()) return;

    loading = true;
    pool.addJob ([this, b64, path, name]
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::shared_ptr<juce::AudioBuffer<float>> buf;
        double sr = 44100.0;
        if (b64.isNotEmpty())
        {
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (b64))
            {
                std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (
                    std::make_unique<juce::MemoryInputStream> (mb.getData(), mb.getSize(), false)));
                if (r) buf = readAll (*r, sr);
            }
        }
        if (! buf && path.isNotEmpty() && juce::File (path).existsAsFile())
        {
            std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (path)));
            if (r) buf = readAll (*r, sr);
        }
        if (buf) installAudio (buf, sr, name, path, false);
        else loading = false;
    });
}

juce::AudioProcessorEditor* FlipGrainProcessor::createEditor() { return new FlipGrainEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FlipGrainProcessor(); }
