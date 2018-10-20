// ClockRecovery
// Rob Dobson 2018
// Currently only works if using Manchester encoding

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

class ClockRecovery
{
private:
	// Samples per symbol and encoding
	int _samplesPerSymbol;
	bool _manchesterEncoding;

	// Current and prev sample count 
	uint32_t _curSampleCount;
	bool _lastTransitionValid;
	uint32_t _lastTransitionSampleCount;

	// Prev sample level - for transition detection
	int _prevSampleLevel;

	// Sample transition times
	struct TransitionType
	{
		uint32_t sampleCount;
		int transitionSamples;
		int modCentrePos;
	};
	static const int MAX_TRANSITION_TIMES_TO_STORE = 5;
	TransitionType _transitionsBuf[MAX_TRANSITION_TIMES_TO_STORE];
	int _transitionBufGetPos;
	int _transitionBufCount;
	int _transitionMinValid;
	int _transitionMaxValid;
	int _transitionAccum;
	int _modCentreAccum;
	int _transitionCentreMin;
	int _transitionCentreMax;

	// Minimum bit duration and sample count when it occurred
	int _minTransitionSamples;
	uint32_t _minTransitionSamplesCount;

	// Adjusted values for Symbols per sample, sample start count, etc
	int _adjustedSymbolEdgeOffset;
	int _adjustedSamplesPerSymbol;

public:
	class ClockDebugVals
	{
	public:
		int transitionInterval;
		int adjSymbolEdgeOffset;
		int adjSamplesPerSymbol;
	};

	ClockRecovery();
	~ClockRecovery();
	void setup(int samplesPerSymbol, bool manchesterEncoding);
	bool newSample(int sampleLevel, ClockDebugVals* pDebugVals = NULL);

private:
	void handleManchesterAdjustments(uint32_t sampleCount, int transitionSamples);
};

