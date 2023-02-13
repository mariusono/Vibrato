/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class VibratoAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    VibratoAudioProcessor();
    ~VibratoAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // SET PARAMS
    void set_width_param(float val) { gSweepWidth_slider = val; }
    void set_frequency_param(float val) { gFrequency_slider = val; }
    void set_gVolume_param(float val) { gVolume_slider = val; }

    //==============================================================================
    // FOR PARAMETERS !
    juce::AudioProcessorValueTreeState apvts;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoAudioProcessor)
    
    
    std::vector<float> gDelayBuffer;
    unsigned int gWritePointer = 0;
    unsigned int gOffset = 3;
    //unsigned int gReadPointer = 0;

    float ph; // current LFO phase (between 0-1)
    float inverseSampleRate;

    float mAvg = 0.002; // 2 ms
    float gMaxWidth = 0.4;

    
    float sweepWidth;
    float sweepWidth_prev;

    float frequency;
    
    float gVolume_slider;
    float gFrequency_slider;
    float gFrequency_slider_smoo;
    float gFrequency_slider_smoo_prev;

    float gSweepWidth_slider;
    float gSweepWidth_slider_smoo;
    float gSweepWidth_slider_smoo_prev;
    
    int flagSwitch = 0;
    float countSwitch = 0;
    float countSwitchMax = 256;
    float countSwitchTime = 0.2; // s

    // AUDIO PARAMS
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
        params.push_back(std::make_unique<AudioParameterFloat>("FREQUENCY","Frequency",0.01f,20.0f,2.0f));
        params.push_back(std::make_unique<AudioParameterFloat>("SWEEP_WIDTH","Sweep_Width",0.0005f,0.4f,0.008f));
        params.push_back(std::make_unique<AudioParameterFloat>("VOLUME","Volume",-30.0f,20.0f,0.0f)); // in dB

        return { params.begin(), params.end()};
    }
    
    
};
