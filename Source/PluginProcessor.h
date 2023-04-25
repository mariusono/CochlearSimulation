/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "BiquadFilter.h"


//==============================================================================
/**
*/
class CochlearSimulationAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CochlearSimulationAudioProcessor();
    ~CochlearSimulationAudioProcessor() override;

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

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CochlearSimulationAudioProcessor)
    
//    int BUFFER_SIZE = 262144;
    int BUFFER_SIZE = 1024;
    
    std::vector<std::vector<float>> gOutputBuffer_eachChannel; // delay buffer from input
    std::vector<float> maxPeakValues; // delay buffer from input
    std::vector<float> outSynthesis; // delay buffer from input

    int gOutputWinWritePointer, gOutputWinReadPointer;
        
    const void * filterCoeff_file;
    size_t filterCoeff_file_size;
    const void * low_pass_filterCoeff_file;
    size_t low_pass_filterCoeff_file_size;
    
    std::vector<float> filterCoeff_values;
    std::vector<std::vector<float>> filterCoeff_values_sep; // noChannels x (filterOrder*6)

    std::vector<float> low_filterCoeff_values;

    
    int noChannels; // 16
    int noActiveElectrodes; // 6
    int filterOrder; // 3
    int low_filterOrder; // 2
    
    float windowTime;
    int windowSize;
    int hopCounter;
    
    std::vector<std::vector<float>> bandPass_IRs; // noChannels x windowSize. to be calculated in prepareToPlay

    // Create bandpass filters
//    std::vector<std::unique_ptr<BiquadFilter>> bandpassFilters; // noChannels x filterOrder
//    std::vector<std::unique_ptr<BiquadFilter>> lowPassFilters; // noChannels x lowPass_filterOrder

    std::vector<std::vector<std::unique_ptr<BiquadFilter>>> bandpassFilters; // noChannels x filterOrder
    std::vector<std::vector<std::unique_ptr<BiquadFilter>>> lowPassFilters; // noChannels x filterOrder

    
    float inVal;
    
    std::vector<std::vector<float>> out_mat; // noChannels x filterOrder

    std::vector<std::vector<float>> out_mat_rect; // noChannels x filterOrder
    
    std::vector<std::vector<float>> out_mat_low; // noChannels x filterOrder
    
    bool flagImpulse = true;
    
};
