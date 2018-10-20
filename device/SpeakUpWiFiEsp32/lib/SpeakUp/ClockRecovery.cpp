// ClockRecovery
// Rob Dobson 2018
// Currently only works if using Manchester encoding

#include "ClockRecovery.h"
#include <cmath>

ClockRecovery::ClockRecovery()
{
	// Samples and transitions
	_curSampleCount = 0;
	_lastTransitionSampleCount = 0;
	_prevSampleLevel = 0;
	_manchesterEncoding = true;
	_samplesPerSymbol = 1;

	// Transition buffer
	_transitionBufGetPos = 0;
	_transitionBufCount = 0;
	_transitionMinValid = 0;
	_transitionMaxValid = 0;
	_transitionCentreMin = 0;
	_transitionCentreMax = 0;
	_transitionAccum = 0;
	_modCentreAccum = 0;

	// Adjusted timing
	_adjustedSymbolEdgeOffset = -1;
	_adjustedSamplesPerSymbol = 1;
}

ClockRecovery::~ClockRecovery()
{
}

void ClockRecovery::setup(int samplesPerSymbol, bool manchesterEncoding)
{
	// Store settings
	_samplesPerSymbol = samplesPerSymbol;
	_adjustedSamplesPerSymbol = samplesPerSymbol;
	_manchesterEncoding = manchesterEncoding;
	_lastTransitionValid = false;
	_adjustedSymbolEdgeOffset = -1;

	// Calculate acceptable transition periods
	if (manchesterEncoding)
	{
		// Only adjust timing based on long transitions (alternating 1s and 0s)
		// As these transitions identify the centre of the manchester symbol
		_transitionMinValid = int(_samplesPerSymbol * 0.9);
		_transitionMaxValid = int(_samplesPerSymbol * 1.1);
		_transitionCentreMin = int(_samplesPerSymbol * 0.25);
		_transitionCentreMax = int(_samplesPerSymbol * 0.75);
	}
}

bool ClockRecovery::newSample(int sampleLevel, ClockDebugVals* pDebugVals)
{
	// See if transition occurred
	bool bitSamplePoint = false;
	bool signalTransition = sampleLevel != _prevSampleLevel;
	if (signalTransition)
	{
		// Find transition interval
		int transitionInterval = _curSampleCount - _lastTransitionSampleCount;
		if (transitionInterval < 0)
		{
			// Number wrap
			transitionInterval = INT_MAX - _lastTransitionSampleCount + _curSampleCount;
		}

		// Debug
		if (pDebugVals)
			pDebugVals->transitionInterval = transitionInterval;

		// Add transition data
		// Handle manchester encoding
		if (_manchesterEncoding)
		{
			// Update adjustments
			handleManchesterAdjustments(_curSampleCount, transitionInterval);
		
			// See if a bit can be decoded
			if (_adjustedSymbolEdgeOffset >= 0)
			{
				// Get the position of the transition
				int posOfTransition = (_curSampleCount - _adjustedSymbolEdgeOffset) % _adjustedSamplesPerSymbol;

				// See if near the centre of the symbol
				if ((posOfTransition >= _transitionCentreMin) && (posOfTransition <= _transitionCentreMax))
				{
					// Indicate that this is a point to determine the bit value
					bitSamplePoint = true;
				}
			}
		}


		//// Handle manchester encoding
		//if (_manchesterEncoding)
		//{
		//	// Calculate the position of the transition
		//	int positionOfTransitionMult = (_curSampleCount * SAMPLES_PER_SYMBOL_DENOMINATOR) % _samplesPerSymbolCorrectedMult;
		//	if (!_lastTransitionValid)
		//	{
		//		// Assume this first transition is the centre of a manchester symbol
		//		_symbolCentreOffsetMult = positionOfTransitionMult;
		//	}
		//	else
		//	{
		//		//// Handle adjustments to the samples per symbol value
		//		//// Should be somewhere near a multiple of sample rate
		//		//int transPhaseMult = (transitionInterval * 2 * SAMPLES_PER_SYMBOL_DENOMINATOR) % _samplesPerSymbolCorrected;
		//		//// Make phase differences of 180 to 360 degrees negative
		//		//if (transPhaseMult > _samplesPerSymbolCorrected / 2)
		//		//	transPhaseMult = (transPhaseMult - _samplesPerSymbolCorrected) / 2;
		//		//// Apple the correction
		//		//_samplesPerSymbolCorrected += (transPhaseMult / SAMPLES_PER_SYMBOL_DENOMINATOR);

		//		// See if this transition is close to the centre of the symbol
		//		int distToCentreMult = abs(positionOfTransitionMult - _symbolCentreOffsetMult);
		//		if (distToCentreMult < (_samplesPerSymbolCorrectedMult / MANCHESTER_PROXIMITY_TO_CENTRE_FACTOR))
		//		{
		//			// Handle phase adjustments to the sampling point
		//			// Check for transion 1 to 0 (a 1 bit which we know cannot occur more than 6 times in succession)
		//			if (_prevSampleLevel == 1)
		//			{
		//				if (_centreSymbolFallingTransitionCount > MAX_MANCHESTER_FALLING_TRANSITIONS)
		//				{
		//					// Shift the phase by 180 degrees as we are sampling the in wrong place
		//					_curSampleCount += _samplesPerSymbolCorrectedMult / 2 / SAMPLES_PER_SYMBOL_DENOMINATOR;
		//				}
		//			}
		//			else
		//			{
		//				_centreSymbolFallingTransitionCount = 0;
		//			}

		//			// Indicate that this is a point to determine the bit value
		//			bitSamplePoint = true;
		//		}
		//		else
		//		{
		//			// Shift the sampling phase progressively
		//			_curSampleCount += positionOfTransitionMult / SAMPLES_PER_SYMBOL_DENOMINATOR / MANCHESTER_PHASE_SHIFT_RATE;
		//		}
		//	}
		//}

		// Save sample count etc
		_lastTransitionSampleCount = _curSampleCount;
		_lastTransitionValid = true;
	}
	else
	{
		if (pDebugVals)
			pDebugVals->transitionInterval = 0;
	}
	
	// Save sample level and bump the sample count
	_prevSampleLevel = sampleLevel;
	uint32_t prevCount = _curSampleCount;
	_curSampleCount++;
	
	// Check for sample count wrap
	if (_curSampleCount < prevCount)
	{
		// Force recalculation of symbol edge - this will probably result in dropping
		// a bit but is a very rare occurrence (once every 6 days if running constantly
		// at 8KHz sampling)
		_adjustedSymbolEdgeOffset = -1;
	}

	return bitSamplePoint;
}

void ClockRecovery::handleManchesterAdjustments(uint32_t sampleCount, int transitionSamples)
{
	// Check the data falls into the acceptable ranges
	if ((transitionSamples < _transitionMinValid) || (transitionSamples > _transitionMaxValid))
		return;

	// Add to stats of transition info 
	int modCentrePos = _adjustedSamplesPerSymbol / 2;
	if (_adjustedSymbolEdgeOffset > 0)
		modCentrePos = (sampleCount - _adjustedSymbolEdgeOffset) % _adjustedSamplesPerSymbol;
	_transitionAccum += transitionSamples;
	_modCentreAccum += modCentrePos;
	if (_transitionBufCount >= MAX_TRANSITION_TIMES_TO_STORE)
	{
		// Remove sample falling out of ring buffer
		_transitionAccum -= _transitionsBuf[_transitionBufGetPos].transitionSamples;
		_modCentreAccum -= _transitionsBuf[_transitionBufGetPos].modCentrePos;
		_transitionBufGetPos = (_transitionBufGetPos + 1) % MAX_TRANSITION_TIMES_TO_STORE;
		_transitionBufCount--;
	}

	// Add to ring buffer of transition info (cannot be full as either empty or we just removed an element) 
	int putPos = (_transitionBufGetPos + _transitionBufCount) % MAX_TRANSITION_TIMES_TO_STORE;
	_transitionsBuf[putPos].transitionSamples = transitionSamples;
	_transitionsBuf[putPos].sampleCount = sampleCount;
	_transitionsBuf[putPos].modCentrePos = modCentrePos;
	_transitionBufCount++;

	// Update the position of the symbol edge if enough data available (or not previously set)
	if (_adjustedSymbolEdgeOffset < 0)
	{
		_adjustedSymbolEdgeOffset = sampleCount + (transitionSamples / 2);
		if (_adjustedSymbolEdgeOffset > _adjustedSamplesPerSymbol)
			_adjustedSymbolEdgeOffset -= _adjustedSamplesPerSymbol;
	}
	else if (_transitionBufCount == MAX_TRANSITION_TIMES_TO_STORE)
	{
		// Adjust rate and centre position of symbol
		_adjustedSymbolEdgeOffset = sampleCount + (_modCentreAccum / MAX_TRANSITION_TIMES_TO_STORE);
		if (_adjustedSymbolEdgeOffset > _adjustedSamplesPerSymbol)
			_adjustedSymbolEdgeOffset -= _adjustedSamplesPerSymbol;
		//_adjustedSamplesPerSymbol = _transitionAccum / MAX_TRANSITION_TIMES_TO_STORE;
	}
}
