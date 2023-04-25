/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//------------------------------------------------------------
// slice vector
template<typename T>
std::vector<T> slice(std::vector<T> const &v, int m, int n)
{
    auto first = v.cbegin() + m;
    auto last = v.cbegin() + n + 1;
 
    std::vector<T> vec(first, last);
    return vec;
}
//------------------------------------------------------------

template<typename T>
std::vector<size_t> argsort(const std::vector<T> &array) {
    std::vector<size_t> indices(array.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&array](int left, int right) -> bool {
                  // sort indices according to corresponding array element
                  return array[left] < array[right];
              });

    return indices;
}


//==============================================================================
CochlearSimulationAudioProcessor::CochlearSimulationAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

CochlearSimulationAudioProcessor::~CochlearSimulationAudioProcessor()
{
}

//==============================================================================
const juce::String CochlearSimulationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CochlearSimulationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CochlearSimulationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CochlearSimulationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CochlearSimulationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CochlearSimulationAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CochlearSimulationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CochlearSimulationAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CochlearSimulationAudioProcessor::getProgramName (int index)
{
    return {};
}

void CochlearSimulationAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CochlearSimulationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    noChannels = 16;
    noActiveElectrodes = 6;
    filterOrder = 3;
    low_filterOrder = 2;


    inVal = 0;
    
    out_mat.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(filterOrder,0);
        out_mat.push_back(var);
    }
    
    out_mat_low.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(low_filterOrder,0);
        out_mat_low.push_back(var);
    }
    
    out_mat_rect.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(filterOrder,0);
        out_mat_rect.push_back(var);
    }
    
    gOutputBuffer_eachChannel.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(BUFFER_SIZE,0);
        gOutputBuffer_eachChannel.push_back(var);
    }
    
    maxPeakValues.resize(noChannels,0);
    
    windowTime = 0.00580498;
    windowSize = ceil(sampleRate * windowTime);
    hopCounter = 0;
    
    outSynthesis.resize(windowSize,0);

    
    gOutputWinWritePointer = 0;
    gOutputWinReadPointer = 0;
    

    gOutputWinReadPointer = 0;
    
    filterCoeff_file = BinaryData::sos_filter_coeffs_txt;
    filterCoeff_file_size = BinaryData::sos_filter_coeffs_txtSize;

    MemoryInputStream* input = new MemoryInputStream (filterCoeff_file, filterCoeff_file_size, false);
    filterCoeff_values.clear(); // making vector empty here in case prepareToPlay is executed multiple times
    while (! input->isExhausted()) // [3]
    {
        auto line = input->readNextLine();
        float coeffVal = line.getFloatValue();
//        double coeffVal_double = static_cast<double>(coeffVal);
        filterCoeff_values.push_back(coeffVal);
    }
    

    filterCoeff_values_sep.clear(); // making vector empty here in case prepareToPlay is executed multiple times
    for (int iChannel = 0; iChannel < noChannels; iChannel++)
    {
        std::vector<float> vecInterm = slice(filterCoeff_values,0 + iChannel*18, 17 + iChannel*18);
//        slice(filterCoeff_values,0 + iChannel*18, 18 + iChannel*18)
        filterCoeff_values_sep.push_back(vecInterm);
        int ana = 3;
    }
    
    
    low_pass_filterCoeff_file = BinaryData::sos_low_filter_coeffs_txt;
    low_pass_filterCoeff_file_size = BinaryData::sos_low_filter_coeffs_txtSize;
    
    MemoryInputStream* input_low_pass = new MemoryInputStream (low_pass_filterCoeff_file, low_pass_filterCoeff_file_size, false);
    low_filterCoeff_values.clear(); // making vector empty here in case prepareToPlay is executed multiple times
    while (! input_low_pass->isExhausted()) // [3]
    {
        auto line = input_low_pass->readNextLine();
        float coeffVal = line.getFloatValue();
//        double coeffVal_double = static_cast<double>(coeffVal);
        low_filterCoeff_values.push_back(coeffVal);
    }
    
    
    bandPass_IRs.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(windowSize,0);
        bandPass_IRs.push_back(var);
    }
    
    bandpassFilters.clear();
    for (int iChannel = 0; iChannel < noChannels; iChannel++)
    {
        std::vector<std::unique_ptr<BiquadFilter>> bandpassFilters_interim;
        for (int iOrder = 0; iOrder < filterOrder; iOrder++)
        {
            bandpassFilters_interim.push_back(std::make_unique<BiquadFilter>(filterCoeff_values_sep[iChannel][0+iOrder*6],
                                                                     filterCoeff_values_sep[iChannel][1+iOrder*6],
                                                                     filterCoeff_values_sep[iChannel][2+iOrder*6],
                                                                     filterCoeff_values_sep[iChannel][3+iOrder*6],
                                                                     filterCoeff_values_sep[iChannel][4+iOrder*6],
                                                                     filterCoeff_values_sep[iChannel][5+iOrder*6]));
        }
        bandpassFilters.push_back(std::move(bandpassFilters_interim));
    }
    
    lowPassFilters.clear();
    for (int iChannel = 0; iChannel < noChannels; iChannel++)
    {
        std::vector<std::unique_ptr<BiquadFilter>> lowPassFilters_interim;
        for (int iOrder = 0; iOrder < low_filterOrder; iOrder++)
        {
            lowPassFilters_interim.push_back(std::make_unique<BiquadFilter>(low_filterCoeff_values[0+iOrder*6],
                                                                   low_filterCoeff_values[1+iOrder*6],
                                                                   low_filterCoeff_values[2+iOrder*6],
                                                                   low_filterCoeff_values[3+iOrder*6],
                                                                   low_filterCoeff_values[4+iOrder*6],
                                                                   low_filterCoeff_values[5+iOrder*6]));
        }
        lowPassFilters.push_back(std::move(lowPassFilters_interim));
    }
    
    
    for (int i_ir_sample = 0; i_ir_sample < windowSize; i_ir_sample++ )
    {
        // audio IN
        if (i_ir_sample == 0){
            inVal = 1;
        }
        else{
            inVal = 0;
        }
        
        for (int iChannel = 0; iChannel < noChannels; iChannel++)
        {
            for (int iOrder = 0; iOrder < filterOrder; iOrder++)
            {
                if (iOrder == 0)
                {
                    out_mat[iChannel][iOrder] = bandpassFilters[iChannel][iOrder]->processSample(inVal);
                }
                else
                {
                    out_mat[iChannel][iOrder] = bandpassFilters[iChannel][iOrder]->processSample(out_mat[iChannel][iOrder-1]);
                }
            }
        }

        // Storing in large buffer.
        for (int iChannel = 0; iChannel < noChannels; iChannel++)
        {
            bandPass_IRs[iChannel][i_ir_sample] = out_mat[iChannel][filterOrder-1];
            
        }
    }
    
    // Reinitialize these..
    inVal = 0;
    
    out_mat.clear();
    for (int i = 0; i < noChannels; i++)
    {
        std::vector<float> var(filterOrder,0);
        out_mat.push_back(var);
    }
    
    int abababa = 3;
    
    
}

void CochlearSimulationAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CochlearSimulationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void CochlearSimulationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float* const outputL = buffer.getWritePointer(0);
    float* const outputR = buffer.getWritePointer(1);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            if (channel == 0)
            {
                auto* input = buffer.getWritePointer (channel);
                
                                            
                // Current input sample
//
                // audio IN
                inVal = input[n];
                
//                // Random noise
//                inVal = ((float) rand() / (RAND_MAX)) * 2 - 1;
                
//                // IMPULSE
//                if (flagImpulse){
//                    inVal = 1.0f;
//                    flagImpulse = false;
//                }
//                else
//                {
//                    inVal = 0;
//                }
                                    
                for (int iChannel = 0; iChannel < noChannels; iChannel++)
                {
                    for (int iOrder = 0; iOrder < filterOrder; iOrder++)
                    {
                        if (iOrder == 0)
                        {
                            out_mat[iChannel][iOrder] = bandpassFilters[iChannel][iOrder]->processSample(inVal);
                        }
                        else
                        {
                            out_mat[iChannel][iOrder] = bandpassFilters[iChannel][iOrder]->processSample(out_mat[iChannel][iOrder-1]);
                        }
                    }
                }
                
                // Rectify output of bandpasses.. NEED NEW ARRAYS !
                for (int iChannel = 0; iChannel < noChannels; iChannel++)
                {
                    out_mat_rect[iChannel][filterOrder-1] = abs(out_mat[iChannel][filterOrder-1]); // this doesn't have to be noChannels x filterOrder but noChannels x 1 !
                }
                
                // Lowpassing each channel of the RECTIFIED outs..
                for (int iChannel = 0; iChannel < noChannels; iChannel++)
                {
                    for (int iOrder = 0; iOrder < low_filterOrder; iOrder++)
                    {
                        if (iOrder == 0)
                        {
                            out_mat_low[iChannel][iOrder] = lowPassFilters[iChannel][iOrder]->processSample(out_mat_rect[iChannel][filterOrder-1]);
                        }
                        else
                        {
                            out_mat_low[iChannel][iOrder] = lowPassFilters[iChannel][iOrder]->processSample(out_mat_low[iChannel][iOrder-1]);
                        }
                    }
                }
                
                // Storing in large buffer.
                for (int iChannel = 0; iChannel < noChannels; iChannel++)
                {
                    gOutputBuffer_eachChannel[iChannel][gOutputWinWritePointer] = out_mat_low[iChannel][low_filterOrder-1];
                }
                                
                float outFinal = outSynthesis[gOutputWinReadPointer] * 2009.1422;
                if (hopCounter == windowSize)
                {
                    // Reset outSynthesis to 0 !
                    std::fill(outSynthesis.begin(), outSynthesis.end(), 0);

                    // extract the max peaks at middle of window..
                    for (int iChannel = 0; iChannel < noChannels; iChannel++)
                    {
                        int indexSel = (gOutputWinWritePointer - static_cast<int>(windowSize/2) + BUFFER_SIZE) % BUFFER_SIZE;
                        
                        maxPeakValues[iChannel] = gOutputBuffer_eachChannel[iChannel][indexSel];
                    }
                    
                    // this could be done differently..
                    auto vals = argsort(maxPeakValues);
                    std::vector<int> vals_int;
                    for (int iVals = 0; iVals<noChannels; iVals++)
                    {
                        vals_int.push_back(static_cast<int>(vals[iVals]));
                    }

                    for (int iSyn = 0; iSyn<windowSize; iSyn++)
                    {
                        for (int iSel = noChannels-noActiveElectrodes; iSel<noChannels;iSel++)
                        {
                            outSynthesis[iSyn] = outSynthesis[iSyn] + maxPeakValues[vals_int[iSel]] * bandPass_IRs[vals_int[iSel]][iSyn];
                        }
                    }
                    hopCounter = 0; // reset hop counter..
                }
                

                hopCounter = hopCounter + 1;
                
//                Logger::getCurrentLogger()->outputDebugString("in is " + String(inVal) + ".");
//                Logger::getCurrentLogger()->outputDebugString("out is " + String(outFinal) + ".");
                outputL[n] = outFinal;
                
                if (abs(outputL[n]) > 1)
                {
                    Logger::getCurrentLogger()->outputDebugString("Output left is too loud!");
                }
                // mono..
                outputR[n] = outputL[n];

                // update pointers..
                
                gOutputWinWritePointer = gOutputWinWritePointer + 1;
                if (gOutputWinWritePointer >= BUFFER_SIZE)
                {
                    gOutputWinWritePointer = 0;
                }
                
                gOutputWinReadPointer = gOutputWinReadPointer + 1;
                if (gOutputWinReadPointer >= windowSize)
                {
                    gOutputWinReadPointer = 0;
                }
            }
        }
    }
    
}

//==============================================================================
bool CochlearSimulationAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CochlearSimulationAudioProcessor::createEditor()
{
    return new CochlearSimulationAudioProcessorEditor (*this);
}

//==============================================================================
void CochlearSimulationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CochlearSimulationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CochlearSimulationAudioProcessor();
}
