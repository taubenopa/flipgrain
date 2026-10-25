#pragma once
#include "Common.h"
#include "Params.h"
#include "Bank.h"
#include "Engine.h"

class FlipGrainProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::AsyncUpdater,
                           private juce::Timer
{
public:
    static constexpr int kVoices = 16;
    static constexpr int kViz = 96;

    FlipGrainProcessor();
    ~FlipGrainProcessor() override;

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "FlipGrain"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- Bedienung (Message-Thread)
    void loadSampleFile (const juce::File& f);
    void flipSample();
    void resetFlip();
    bool exportPatternTo (const juce::File& f);
    std::shared_ptr<const fg::Bank> getBank() const;
    float getSliceSemis (const fg::Bank& b, int sliceIdx) const;
    juce::String getStatus() const;
    bool isLoading() const { return loading.load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // --- Visualisierung (Audio -> UI)
    std::array<std::atomic<float>, kViz> vizX {}, vizA {};
    std::array<std::atomic<float>, kVoices> playheads {};
    std::atomic<uint64_t> activeSliceMask { 0 };
    std::atomic<float> meterL { 0.f }, meterR { 0.f };

private:
    struct Ptrs
    {
        std::atomic<float> *mode, *engine, *sliceMode, *sens, *base, *key, *scale, *retune, *flipAmt, *region, *loop,
            *position, *scan, *grainSize, *density, *spray, *jitter, *spread, *revProb, *shape,
            *attack, *decay, *sustain, *release, *lp, *hp, *res, *revMix, *revSize, *tune, *gain;
    } p {};

    void parameterChanged (const juce::String& id, float) override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    fg::Snapshot makeSnapshot() const;
    void handleMidi (const juce::MidiMessage&, fg::Snapshot&, const juce::ADSR::Parameters&);
    void noteOn (int note, float vel, const fg::Snapshot&, const juce::ADSR::Parameters&);
    fg::Voice* allocateVoice();
    void renderVoices (float* L, float* R, int n, const fg::Snapshot&);
    void updateViz();

    // Worker-Thread
    void installAudio (std::shared_ptr<juce::AudioBuffer<float>> buf, double sr, const juce::String& name,
                       const juce::String& path, bool resetFlipState);
    void rebuild();
    void setBank (std::shared_ptr<const fg::Bank> b);
    void setStatus (const juce::String&);

    // Bank-Verwaltung (Graveyard: letzte Referenz wird nie im Audio-Thread freigegeben)
    mutable juce::SpinLock bankLock;
    std::shared_ptr<const fg::Bank> current;
    std::vector<std::shared_ptr<const fg::Bank>> graveyard;
    std::atomic<uint32_t> bankVersion { 0 };

    // Audio-Thread
    std::shared_ptr<const fg::Bank> audioBank;
    uint32_t audioBankVersion = 0;
    std::array<fg::Voice, kVoices> voices;
    uint64_t tickCounter = 0;
    double bendSemis = 0.0;
    juce::Random rng;
    std::vector<float> monoScratch;

    juce::dsp::StateVariableTPTFilter<float> lpFilter, hpFilter;
    juce::Reverb reverb;
    juce::SmoothedValue<float> gainSm;
    double currentSR = 44100.0;

    // Daten / Worker
    juce::CriticalSection dataLock;
    std::shared_ptr<const juce::AudioBuffer<float>> audioSource;
    double sourceRate = 44100.0;
    juce::String sampleName, samplePath, statusText;
    juce::MemoryBlock flacCache;
    std::shared_ptr<fg::Bank> baseBank;           // nur Worker-Thread
    std::atomic<uint32_t> flipSeed { 0 };
    std::atomic<float> flipAmtUsed { 0.6f };
    std::atomic<int> flipKeyUsed { 0 }, flipScaleUsed { 0 };
    std::atomic<bool> loading { false };
    std::atomic<int> rebuildCountdown { 0 };
    int housekeeping = 0;
    juce::ThreadPool pool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlipGrainProcessor)
};
