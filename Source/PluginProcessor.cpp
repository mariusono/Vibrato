/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VibratoAudioProcessor::VibratoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
}

VibratoAudioProcessor::~VibratoAudioProcessor()
{
}

//==============================================================================
const juce::String VibratoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VibratoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VibratoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VibratoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VibratoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VibratoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VibratoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VibratoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VibratoAudioProcessor::getProgramName (int index)
{
    return {};
}

void VibratoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VibratoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    // Resize delay buffer
    gDelayBuffer.resize(ceil(gMaxWidth*1.5 * sampleRate));
//    gDelayBuffer.resize(ceil(5 * sampleRate));

    
    inverseSampleRate = 1/sampleRate;
    
    
    sweepWidth = 0.008;
    sweepWidth_prev = 0.008;

    frequency = 2;
    
    gFrequency_slider = 2;
    gSweepWidth_slider = 0.008;
    gVolume_slider = 1.0;
    
    
    gFrequency_slider_smoo = 2;
    gFrequency_slider_smoo_prev = 2;
    
    gSweepWidth_slider_smoo = 0.008;
    gSweepWidth_slider_smoo_prev = 0.008;
    
    ph = 0; // phase increment
    
    flagSwitch = 0;
    countSwitchMax = floorf(countSwitchTime*(float)sampleRate);
}

void VibratoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VibratoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VibratoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float* const outputL = buffer.getWritePointer(0);
    float* const outputR = buffer.getWritePointer(1);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            if (channel == 0)
            {
                auto* input = buffer.getWritePointer (channel);
                
                // Current input sample
                float in = input[i];
                
                float interpolatedSample = 0;
                float interpolatedSample_prev = 0;
                float interpolatedSample_comb  = 0;

                gFrequency_slider_smoo = (1-0.95)*gFrequency_slider + 0.95*gFrequency_slider_smoo_prev;
                gFrequency_slider_smoo_prev = gFrequency_slider_smoo;
                
                frequency = gFrequency_slider_smoo;
                
                gSweepWidth_slider_smoo = (1-0.95)*gSweepWidth_slider + 0.95*gSweepWidth_slider_smoo_prev;
                gSweepWidth_slider_smoo_prev = gSweepWidth_slider_smoo;
                                
                if (flagSwitch == 0)
                {
//                    if (gSweepWidth_slider != sweepWidth)
                    if (gSweepWidth_slider_smoo != sweepWidth)
                    {
                        flagSwitch = 1;
                        sweepWidth_prev = sweepWidth;
//                        sweepWidth = gSweepWidth_slider;
                        sweepWidth = gSweepWidth_slider_smoo;
                    }
                }
                
// First version just minimizes the initial delay. Perhaps try to offset the sin wave, i.e. change phase so that is starts at a value where it is 0 !
                float currentDelay = sweepWidth * (0.5 + 0.5 * sinf(2.0 * M_PI * ph)); // ph is f/fs basically. that 0.5 means max width can be 0.5
//                float currentDelay =  (gMaxWidth + sweepWidth * sinf(2.0 * M_PI * ph)); // ph is f/fs basically. that 0.5 means max width can be 0.5
                
                float gReadPointer = fmodf((float)gWritePointer - (float)(currentDelay * getSampleRate()) + (float)gDelayBuffer.size() - (float)gOffset, (float)gDelayBuffer.size());
                      
                ////  Linear interpolation
                //float fraction = gReadPointer - floorf(gReadPointer);
                //int previousSample = (int)floorf(gReadPointer);
                //int nextSample = (previousSample + 1) % gDelayBuffer.size();
                //interpolatedSample = fraction*gDelayBuffer[nextSample] + (1.0f-fraction)*gDelayBuffer[previousSample];
                     
                     
                // Cubic interpolation
                int index = (int)floorf(gReadPointer);
                float alpha = gReadPointer - index;
                
                int index_m1 = (index-1) % gDelayBuffer.size();
                int index_p1 = (index+1) % gDelayBuffer.size();
                int index_p2 = (index+2) % gDelayBuffer.size();
                
                interpolatedSample = ( alpha*(alpha-1)*(alpha-2)*gDelayBuffer[index_m1]/(-6)
                                         + (alpha-1)*(alpha+1)*(alpha-2)*gDelayBuffer[index]/2
                                         + alpha*(alpha+1)*(alpha-2)*gDelayBuffer[index_p1]/(-2)
                                         + alpha*(alpha+1)*(alpha-1)*gDelayBuffer[index_p2]/(6) );
                     
                
                float currentDelay_prev = sweepWidth_prev * (0.5 + 0.5 * sinf(2.0 * M_PI * ph)); // ph is f/fs basically.
                
                gReadPointer = fmodf((float)gWritePointer - (float)(currentDelay_prev * getSampleRate()) + (float)gDelayBuffer.size() - (float)gOffset, (float)gDelayBuffer.size());
                      
                     
                // Cubic interpolation
                index = (int)floorf(gReadPointer);
                alpha = gReadPointer - index;
                
                index_m1 = (index-1) % gDelayBuffer.size();
                index_p1 = (index+1) % gDelayBuffer.size();
                index_p2 = (index+2) % gDelayBuffer.size();
                
                interpolatedSample_prev = ( alpha*(alpha-1)*(alpha-2)*gDelayBuffer[index_m1]/(-6)
                                         + (alpha-1)*(alpha+1)*(alpha-2)*gDelayBuffer[index]/2
                                         + alpha*(alpha+1)*(alpha-2)*gDelayBuffer[index_p1]/(-2)
                                         + alpha*(alpha+1)*(alpha-1)*gDelayBuffer[index_p2]/(6) );
                
                if (flagSwitch==1)
                {
                    
                    Logger::getCurrentLogger()->outputDebugString("flagSwitch is " + String(flagSwitch) + ".");
                    
                    double fac1 = sqrt((1 - countSwitch/countSwitchMax));
                    double fac2 = sqrt((countSwitch/countSwitchMax));
                    
                    interpolatedSample_comb = interpolatedSample_prev * fac1 + interpolatedSample*fac2;
                    
                    countSwitch = countSwitch + 1;
                    if (countSwitch == countSwitchMax)
                    {
                        countSwitch = 0;
                        flagSwitch = 0;
                        sweepWidth_prev = sweepWidth; // update sweepWidth_prev
                        
//                        Logger::getCurrentLogger()->outputDebugString("flagSwitch is " + String(flagSwitch) + ".");
                    }
                }
                else if (flagSwitch==0)
                {
                    interpolatedSample_comb = interpolatedSample;
                }
                

                gDelayBuffer[gWritePointer] = in;
                    
                gWritePointer++;
                if(gWritePointer>=gDelayBuffer.size())
                {
                    gWritePointer = 0;
                }
        
                // Update the LFO phase, keeping it in the range 0-1
                ph += frequency*inverseSampleRate;
                if(ph >= 1.0)
                {
                    ph -= 1.0;
                }
                
                outputL[i] = interpolatedSample_comb * gVolume_slider;
//                outputL[i] = interpolatedSample;
                outputR[i] = outputL[i];

            }
        }
    }
    
}

//==============================================================================
bool VibratoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VibratoAudioProcessor::createEditor()
{
    return new VibratoAudioProcessorEditor (*this);
}

//==============================================================================
void VibratoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VibratoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoAudioProcessor();
}
