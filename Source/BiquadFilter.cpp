/*
  ==============================================================================

    BiquadFilter.cpp
    Created: 25 Apr 2023 11:23:53am
    Author:  Marius Onofrei

  ==============================================================================
*/

#include <JuceHeader.h>
#include "BiquadFilter.h"

//==============================================================================
// Constructor
BiquadFilter::BiquadFilter(float b1, float b2, float b3, float a1, float a2, float a3)
{
    this->a1 = a1;
    this->a2 = a2;
    this->a3 = a3;
    this->b1 = b1;
    this->b2 = b2;
    this->b3 = b3;
    
    inVal_m1 = 0;
    inVal_m2 = 0;
    outVal_m1 = 0;
    outVal_m2 = 0;
}

// Destructor
BiquadFilter::~BiquadFilter(){
}


void BiquadFilter::resetState(){
    inVal_m1 = 0;
    inVal_m2 = 0;
    outVal_m1 = 0;
    outVal_m2 = 0;
}

float BiquadFilter::processSample(float inVal){
//    outVal = b1 * inVal + b2 * inVal_m1 + b3 * inVal_m2 - a2*outVal_m1 - a3*outVal_m2;
    outVal = (b1 * inVal + b2 * inVal_m1 + b3 * inVal_m2 - a2*outVal_m1 - a3*outVal_m2) / a1;

    // update samples
    inVal_m2 = inVal_m1;
    inVal_m1 = inVal;
    
    outVal_m2 = outVal_m1;
    outVal_m1 = outVal;
    
    return outVal;
}
