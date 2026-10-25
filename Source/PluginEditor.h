#pragma once
#include "PluginProcessor.h"
#include "UI/Widgets.h"
#include "UI/Views.h"

class FlipGrainEditor : public juce::AudioProcessorEditor,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit FlipGrainEditor (FlipGrainProcessor&);
    ~FlipGrainEditor() override;

    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;

private:
    static constexpr int kW = 1180, kH = 760;

    struct Panel { juce::Rectangle<float> r; juce::String title; juce::Colour accent; };
    struct Caption { juce::String text; juce::Rectangle<float> r; };

    class Backdrop : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        std::vector<Panel> panels;
        std::vector<Caption> captions;
        juce::String status;
    };

    void timerCallback() override;
    void layoutAll();
    void pasteFromClipboard();
    void tryLoad (const juce::File&);
    void chooseFile();
    void toast (const juce::String&);
    static bool isAudioFile (const juce::File&);

    FlipGrainProcessor& proc;
    fg::ui::NeonLookAndFeel laf;

    Backdrop root;
    fg::ui::WaveformView waveform;
    fg::ui::KeyboardView keyboard;
    fg::ui::FlipButton flipButton;
    fg::ui::MidiPad midiPad;
    fg::ui::MeterView meter;
    fg::ui::GrainView grainView;

    juce::TextButton loadButton { "LOAD" }, pasteButton { "PASTE" }, resetButton { "RESET" };
    juce::ComboBox keyBox, scaleBox, sliceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt, sliceAtt;
    std::vector<std::unique_ptr<fg::ui::Knob>> knobs;
    std::vector<std::unique_ptr<fg::ui::Segmented>> segs;

    std::unique_ptr<juce::FileChooser> chooser;
    std::shared_ptr<const fg::Bank> lastBank;
    juce::String toastText;
    juce::uint32 toastUntil = 0;
    int tempCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlipGrainEditor)
};
