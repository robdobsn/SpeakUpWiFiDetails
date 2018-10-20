// Example ESP32 to send WiFi credentials over audio
// Rob Dobson 2018

#include <Arduino.h>
#include <driver/adc.h>

#include "SpeakUp.h"

// Handler for audio comms
SpeakUp speakUp;

// const int MAX_SAMPLES = 16000;
// int16_t soundBuf[MAX_SAMPLES];
// volatile int curPos = 0;

// ESP32 timer pointer
hw_timer_t *_pHwTimer = NULL;
// volatile int lastSample = 0;

// Timer interrupt for ADC and comms
void IRAM_ATTR onTimer() 
{
    // Get analog from pin 36
    int adcVal = adc1_get_raw(ADC1_CHANNEL_0); 
    int mappedValue = map(adcVal, 0 , 4095, -32767, 32767);

    // lastSample = mappedValue;
    // if (curPos < MAX_SAMPLES)
    // {
    //     soundBuf[curPos++] = mappedValue;
    // }

    // Send to audio
    speakUp.decodeProcessSample(mappedValue);
}

void setESP32TimerForADC()
{
    // Configure ADC1
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
    
    // Timer prescaled to 80 (assumes ESP32 timer base clock is 80MHz)
    _pHwTimer = timerBegin(1, 80, true);
    timerAttachInterrupt(_pHwTimer, &onTimer, true);

    // Set timer alarm to result in 8KHz sampling
    timerAlarmWrite(_pHwTimer, 125, true);
    timerAlarmEnable(_pHwTimer);
}

void setup() {
    Serial.begin(115200);
    speakUp.setup();
    setESP32TimerForADC();
    Serial.println("Waiting for audio ...\n");
}

void loop() {
    // See if anything received
    String msg;
    if (speakUp.decodeGetMessage(msg))
    {
        Serial.println(msg);
    }
    else
    {
        // if (curPos == MAX_SAMPLES)
        // {
        //     Serial.printf("\n\nSAMPLES\n\n\n\n");
        //     for (int i = 0; i < MAX_SAMPLES; i++)
        //     {
        //         Serial.printf("%d,", soundBuf[i]);
        //     }
        //     Serial.printf("\n\n\n\n");
        //     curPos = 0;
        // }
    }
}
