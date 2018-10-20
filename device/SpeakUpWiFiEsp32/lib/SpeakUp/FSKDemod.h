// FSKDemod
// Rob Dobson 2018
// Binary FSK demodulator 

#pragma once

#include <stdint.h>
#include <limits.h>
#include <vector>
#include "RingBufferPosn.h"
#include "ClockRecovery.h"

class FSKDemod
{
private:
	// Sample rate, bit rate and symbols
	int _sampleRate;
	int _symbolRate;
	int _numSymbols;
	std::vector<int> _symbolFreqs;
	bool _manchesterCodec;

	// Butterworth 3 pole low-pass filter
	static const int NUM_FILTER_POLES = 3;
	int xv[NUM_FILTER_POLES+1];                        // IIR Filter X cells
	int yv[NUM_FILTER_POLES+1];                        // IIR Filter Y cells

	// Gain and params of filter
	static const int FILTER_INT_MULT = 100;
	static const int FILTER_GAIN = 4;
	static const int FILTER_PARAM_1 = int(0.0562 * FILTER_INT_MULT + 0.5);
	static const int FILTER_PARAM_2 = int(-0.4217 * FILTER_INT_MULT + 0.5);
	static const int FILTER_PARAM_3 = int(0.5772 * FILTER_INT_MULT + 0.5);

	// Previous sample levels
	static const int NUM_SAMPLES_VOTING = 3;
	int _sampleVoting[NUM_SAMPLES_VOTING];

	// Output bit buffer
	RingBufferPosn _rxSymbolFifoPos;
	std::vector<uint8_t> _rxSymbolFifoBuf;

	// Smoothing filters for discrimination
	int _curEnvelopeVal;
	int _envelopePercent;
	int _signalHigh;
	int _signalLow;
	int _peakFollowPer10K;
	int _peakRestPer10K;

	// Current signal level
	int _curSignalLevel;

	// Clock recovery
	ClockRecovery _clockRecovery;

public:

	class FSKDebugVals
	{
	public:
		int inputValue;
		int envelopeValue;
		int signalInstantaneous;
		int curSignalLevel;
		int symbolVal;
		int signalLow;
		int signalHigh;
		ClockRecovery::ClockDebugVals clockVals;
		FSKDebugVals()
		{
			inputValue = 0;
			envelopeValue = 0;
			signalInstantaneous = 0;
			curSignalLevel = 0;
			symbolVal = 0;
			signalLow = 0;
			signalHigh = 0;
		}
	};

	// Constructor
	FSKDemod(int rxFifoLen) : _rxSymbolFifoPos(rxFifoLen)
	{
		// Clear
		_rxSymbolFifoBuf.resize(rxFifoLen);
		_curEnvelopeVal = 0;
		_envelopePercent = 20;
		_peakFollowPer10K = 500;
		_peakRestPer10K = 10;
		_signalHigh = 0;
		_signalLow = 32767;
		_manchesterCodec = true;
		_curSignalLevel = 0;
	}

	// Setup
	void setup(int sampleRate, int symbolRate, int symbolFreqHigh, 
				int symbolFreqLow, bool manchesterCodec);

	// Process a single sample
	void processSample(int currentSample, FSKDebugVals* pDebugVals = NULL);

	// Get a received bit
	bool getRxBit(int& bitVal);

private:
	// Helpers
	int updateSignalHigh(int curVal);
	int updateSignalLow(int curVal);

};
