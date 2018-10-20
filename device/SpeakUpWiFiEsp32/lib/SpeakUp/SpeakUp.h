// SpeakUp - send messages over audio
// Rob Dobson 2018

#pragma once

#include <functional>
#include "FSKDemod.h"
#include "FSKMod.h"
#include "MiniHDLC.h"

class SpeakUp
{
private:
	// Received frame
	volatile bool _rxReady = false;
	String _rxMessage;

	// Mod/Demod & data link
	FSKMod _fskMod;
	FSKDemod _fskDemod;
	MiniHDLC _hdlc;

	// Settings
	static const int SAMPLE_RATE_PER_SEC = 8000;
	static const int TX_BITS_FIFO_LEN = 1000;
	static const int RX_SAMPLES_FIFO_LEN = 4000;
	static const int SYMBOL_RATE_PER_SEC = 100;
	static const int SYMBOL_FREQ_HIGH = 2000;
	static const int SYMBOL_FREQ_LOW = 1000;

public:
	SpeakUp() :
		_fskMod(TX_BITS_FIFO_LEN),
		_fskDemod(RX_SAMPLES_FIFO_LEN),
		_hdlc(std::bind(&SpeakUp::putBit, this, std::placeholders::_1), 
					std::bind(&SpeakUp::rxFrame, this, std::placeholders::_1, std::placeholders::_2),
					 true, true)
	{
		_rxReady = false;
		setup();
	}

	// Setup library
	void setup()
	{
		_fskMod.setup(SAMPLE_RATE_PER_SEC, SYMBOL_RATE_PER_SEC, SYMBOL_FREQ_HIGH, SYMBOL_FREQ_LOW, true);
		_fskDemod.setup(SAMPLE_RATE_PER_SEC, SYMBOL_RATE_PER_SEC, SYMBOL_FREQ_HIGH, SYMBOL_FREQ_LOW, true);
	}

	// Generate audio samples for a message
	void encodeMessageToSamples(const char* msg)
	{
		_fskMod.clear();
		_fskMod.addPreamble();
		_hdlc.sendFrame((const uint8_t*)msg, strlen(msg));
		_fskMod.addPostamble();
	}

	// Get next audio sample for message
	// Returns false if no sample available
	bool encodeGetSample(int& sampleValue)
	{
		return _fskMod.getSample(sampleValue);
	}

	// Process an audio sample
	void decodeProcessSample(int sampleVal, FSKDemod::FSKDebugVals* pDebugVals = NULL)
	{
		// Process sample
		_fskDemod.processSample(sampleVal, pDebugVals);
		
		// Get any bits received and send to hdlc
		int bitVal = 0;
		if (_fskDemod.getRxBit(bitVal))
			_hdlc.handleBit(bitVal);
	}

	// Get a message if available
	bool decodeGetMessage(String& msg)
	{
		if (_rxReady)
		{
			msg = _rxMessage;
			_rxReady = false;
			return true;
		}
		return false;
	}

	// Clear ready message
	void decodeClearMessage()
	{
		_rxReady = false;
	}

private:
	// Callback from HDLC encode when a bit is ready to be "sent" 
	void putBit(uint8_t bit)
	{
		_fskMod.addSymbol(bit);
	}

	// Callback from HDLC decode when a frame is complete
	void rxFrame(const uint8_t *framebufferNullTerminated, int framelength)
	{
		// Check if previous message not handled
		if (_rxReady)
			return;
			
		// Store the message as a string
		_rxMessage = (const char*) framebufferNullTerminated;
		_rxReady = true;
	}
};
