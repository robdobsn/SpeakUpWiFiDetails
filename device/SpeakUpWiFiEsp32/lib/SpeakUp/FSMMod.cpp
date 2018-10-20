// FSKMod
// Rob Dobson 2018
// Binary FSK modulator

#include "FSKMod.h"
#include <stdlib.h>

void FSKMod::setup(int sampleRate, int symbolRate, int symbolFreqHigh, int symbolFreqLow,
				bool manchesterCodec)
	{
		_sampleRate = sampleRate;
		_symbolRate = symbolRate;
		_numSymbols = 2;
		_symbolFreqs.resize(_numSymbols);
		_symbolFreqs[0] = symbolFreqLow;
		_symbolFreqs[1] = symbolFreqHigh;
		_manchesterCodec = manchesterCodec;
		_generatorCount = 0;
		_generatorBusy = false;
	}

	void FSKMod::clear()
	{
		_txSymbolFifoPos.clear();
		_generatorCount = 0;
		_generatorBusy = false;
	}

	void FSKMod::setPreamble(int lenInSymbols)
	{
		_preambleSymbols = lenInSymbols;
	}

	void FSKMod::setPostamble(int lenInSymbols)
	{
		_postambleSymbols = lenInSymbols;
	}

	void FSKMod::addPreamble()
	{
		// Preamble is alternating 1s and 0s
		for (int i = 0; i < _preambleSymbols; i++)
			addSymbol(i % 2);
	}

	void FSKMod::addPostamble()
	{
		// Postamble is a string of 0s
		for (int i = 0; i < _postambleSymbols; i++)
			addSymbol(0);
	}

	bool FSKMod::addSymbol(int symbol)
	{
		// Check if space in FIFO
		if (_txSymbolFifoPos.canPut())
		{
			// Add symbol
			symbol = symbol % _numSymbols;
			_txSymbolFifoBuf[_txSymbolFifoPos.posToPut()] = symbol;
			_txSymbolFifoPos.hasPut();
			return true;
		}
		return false;
	}

	bool FSKMod::getSample(int& sampleValue)
	{
		// See if we need to start processing another bit
		if (!_generatorBusy && _txSymbolFifoPos.canGet())
		{
			// Get the symbol and start generating
			int symbolVal = _txSymbolFifoBuf[_txSymbolFifoPos.posToGet()];
			_txSymbolFifoPos.hasGot();
			// Start generating
			startSymbol(symbolVal);
		}

		// See if already generating
		if (_generatorBusy)
		{
			// Generate next sample
			int phaseIdx = _generatorCount % (_sinTableLen * 4);
			int lookupIdx = phaseIdx % (_sinTableLen * 2);
			lookupIdx = (lookupIdx >= _sinTableLen) ? (_sinTableLen * 2) - lookupIdx - 1 : lookupIdx;
			sampleValue = _sinTable[lookupIdx] * ((phaseIdx > _sinTableLen * 2) ? -1 : 1);

			// Next
			_generatorCount += _generatorInc;
			_samplesToNextChange--;
			if (_samplesToNextChange <= 0)
			{
				if (_manchesterCodec && (_manchesterPhase == 0))
				{
					setFreq(_symbolFreqs[_curSymbolVal ? 0 : 1]);
					_manchesterPhase = 1;
				}
				else
				{
					_generatorBusy = false;
				}
			}
			return true;
		}
		else
		{
			_generatorCount = 0;
		}
		return false;
	}

	void FSKMod::startSymbol(int symbolValue)
	{
		// Initialse generator
		_manchesterPhase = 0;
		_curSymbolVal = symbolValue;
		setFreq(_symbolFreqs[symbolValue]);

		// Generator now busy
		_generatorBusy = true;
	}

	void FSKMod::setFreq(int freqInHz)
	{
		// Calculate phase increment
		_curFrequency = freqInHz;
		_generatorInc = (_sinTableLen * 4) / (_sampleRate / _curFrequency);

		// Calculate samples to generate before next change
		_samplesToNextChange = _sampleRate / _symbolRate;
		if (_manchesterCodec)
			_samplesToNextChange = _samplesToNextChange / 2;
	}
    