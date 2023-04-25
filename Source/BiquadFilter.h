/*
  ==============================================================================

    BiquadFilter.h
    Created: 25 Apr 2023 11:23:53am
    Author:  Marius Onofrei

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class BiquadFilter  : public juce::Component
{
public:
    BiquadFilter(float b1, float b2, float b3, float a1, float a2, float a3);
    ~BiquadFilter() override;
    
    void resetState();
    float processSample(float inVal);

private:
    
    float inVal_m1, inVal_m2, outVal, outVal_m1, outVal_m2;
    
    float a1,a2,a3,b1,b2,b3;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiquadFilter)

};
