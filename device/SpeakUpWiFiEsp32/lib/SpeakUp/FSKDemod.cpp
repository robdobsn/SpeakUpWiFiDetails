// FSKDemod
// Rob Dobson 2018
// Binary FSK demodulator

#include "FSKDemod.h"
#include <stdlib.h>

// Setup - defining sample rate, etc
void FSKDemod::setup(int sampleRate, int symbolRate, int symbolFreqHigh,
                    int symbolFreqLow, bool manchesterCodec)
{
    _sampleRate = sampleRate;
    _symbolRate = symbolRate;
    _numSymbols = 2;
    _symbolFreqs.resize(_numSymbols);
    _symbolFreqs[0] = symbolFreqLow;
    _symbolFreqs[1] = symbolFreqHigh;
    _manchesterCodec = manchesterCodec;
    _clockRecovery.setup(sampleRate / symbolRate, _manchesterCodec);
}

// Process a single sample
void FSKDemod::processSample(int currentSample, FSKDebugVals *pDebugVals)
{
    // Implementation of Butterworth 3rd Order highpass IIR filter
    // http://www-users.cs.york.ac.uk/~fisher/mkfilter
    // 8KHz sample rate, 1600Hz cutoff
    xv[0] = xv[1];
    xv[1] = xv[2];
    xv[2] = xv[3];
    xv[3] = (currentSample * FILTER_INT_MULT) / FILTER_GAIN;
    yv[0] = yv[1];
    yv[1] = yv[2];
    yv[2] = yv[3];
    yv[3] = (xv[3] - xv[0]) + 3 * (xv[1] - xv[2]) + (FILTER_PARAM_1 * yv[0]) + (FILTER_PARAM_2 * yv[1]) + (FILTER_PARAM_3 * yv[2]);
    yv[3] = yv[3] / FILTER_INT_MULT;

    // Compute abs value
    int outVal = abs(yv[3]);

    // Envelope detect
    updateSignalHigh(outVal);
    updateSignalLow(outVal);
    _curEnvelopeVal = _curEnvelopeVal + ((outVal - _curEnvelopeVal) * _envelopePercent) / FILTER_INT_MULT;
    uint8_t _signalInstantaneous = _curEnvelopeVal > (_signalHigh + _signalLow) / 2;

    // Debug
    if (pDebugVals)
    {
        pDebugVals->symbolVal = -1;
        pDebugVals->inputValue = currentSample;
        pDebugVals->envelopeValue = _curEnvelopeVal;
        pDebugVals->signalInstantaneous = _signalInstantaneous;
        pDebugVals->signalLow = _signalLow;
        pDebugVals->signalHigh = _signalHigh;
    }

    // Voting on the bit value
    int oneCount = 0;
    for (int i = 0; i < NUM_SAMPLES_VOTING - 1; i++)
    {
        _sampleVoting[i] = _sampleVoting[i + 1];
        oneCount += _sampleVoting[i] ? 1 : 0;
    }
    _sampleVoting[NUM_SAMPLES_VOTING - 1] = _signalInstantaneous;

    // Count ones in sequence length
    oneCount += _signalInstantaneous ? 1 : 0;
    if (oneCount == 0)
        _curSignalLevel = 0;
    else if (oneCount == NUM_SAMPLES_VOTING)
        _curSignalLevel = 1;
    if (pDebugVals)
        pDebugVals->curSignalLevel = _curSignalLevel;

    // Recover clock
    bool sampleNow = _clockRecovery.newSample(_curSignalLevel, pDebugVals ? (&pDebugVals->clockVals) : NULL);
    if (sampleNow)
    {
        // Get the bit value
        int symbolValue = _curSignalLevel;

        // Invert if manchester encoding as we are just beyond the centre-symbol transition
        if (_manchesterCodec)
            symbolValue = symbolValue ? 0 : 1;

        // Put value into output buffer
        if (_rxSymbolFifoPos.canPut())
        {
            _rxSymbolFifoBuf[_rxSymbolFifoPos.posToPut()] = symbolValue;
            _rxSymbolFifoPos.hasPut();
        }

        // Debug
        if (pDebugVals)
        {
            pDebugVals->symbolVal = symbolValue;
        }
    }
}

// Get a received bit (if available)
bool FSKDemod::getRxBit(int &bitVal)
{
    if (!_rxSymbolFifoPos.canGet())
        return false;
    bitVal = _rxSymbolFifoBuf[_rxSymbolFifoPos.posToGet()];
    _rxSymbolFifoPos.hasGot();
    return true;
}

// Helper functions
int FSKDemod::updateSignalHigh(int curVal)
{
    int curDiff = _curEnvelopeVal - _signalHigh;
    _signalHigh += (curDiff > 0) ? ((curDiff * _peakFollowPer10K) / 10000) : ((curDiff * _peakRestPer10K) / 10000);
    return _signalHigh;
}

int FSKDemod::updateSignalLow(int curVal)
{
    int curDiff = _curEnvelopeVal - _signalLow;
    _signalLow += (curDiff < 0) ? ((curDiff * _peakFollowPer10K) / 10000) : ((curDiff * _peakRestPer10K) / 10000);
    return _signalLow;
}
