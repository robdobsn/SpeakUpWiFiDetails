// Example ESP32 to send WiFi credentials over audio
// Rob Dobson 2018

#include <Arduino.h>
#include <driver/adc.h>
#include "SpeakUp.h"
#include "Display.h"
#include <WiFi.h>

// Handler for audio comms
SpeakUp speakUp;

// ESP32 timer pointer
hw_timer_t *_pHwTimer = NULL;

// ESP32 ADC1 channel for analog audio (ADC)
// This is of the form ADC1_CHANNEL_N
// Where N is:0,1,2,3,4,5,6,7 for ESP32 pins 36,37,38,39,32,33,34,35 respectively
const adc1_channel_t ADC_INPUT_CHANNEL = ADC1_CHANNEL_7;

// Optional display driver
Display display;

// Timer interrupt for ADC and comms
void IRAM_ATTR onTimer() 
{
    // Get analog from pin
    int adcVal = adc1_get_raw(ADC_INPUT_CHANNEL); 
    int mappedValue = map(adcVal, 0 , 4095, -32767, 32767);

    // Send to audio
    speakUp.decodeProcessSample(mappedValue);
}

void setESP32TimerForADC()
{
    // Configure ADC1
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC_INPUT_CHANNEL, ADC_ATTEN_11db);
    
    // Timer prescaled to 80 (assumes ESP32 timer base clock is 80MHz)
    _pHwTimer = timerBegin(1, 80, true);
    timerAttachInterrupt(_pHwTimer, &onTimer, true);

    // Set timer alarm to result in 8KHz sampling
    timerAlarmWrite(_pHwTimer, 125, true);
    timerAlarmEnable(_pHwTimer);
}

int prevWiFiStatus = -1;

void setup() {
    Serial.begin(115200);
    speakUp.setup();
    setESP32TimerForADC();
    display.welcome(ADC_INPUT_CHANNEL);
    Serial.println("Waiting for audio ...\n");
}

void loop() {
    // See if anything received
    String msg;
    if (speakUp.decodeGetMessage(msg))
    {
        // Show progress
        Serial.println(msg);

        // Extract SSID & PW
        int ssidPos = msg.indexOf("\"s\":");
        if (ssidPos < 0)
            return;
        int ssidEnd = msg.indexOf("\"", ssidPos+5);
        if (ssidEnd < 0)
            return;
        int pwPos = msg.indexOf("\"p\":");
        if (pwPos < 0)
            return;
        int pwEnd = msg.indexOf("\"", pwPos+5);
        if (pwEnd < 0)
            return;
        String ssid = msg.substring(ssidPos+5, ssidEnd);
        String pw = msg.substring(pwPos+5, pwEnd);

        // Show progress
        display.showSSID(ssid);

        // Stop timer as it conflicts with WiFi on the WEMOS platform
        Serial.println("Stopping timer\n");
        timerAlarmDisable(_pHwTimer);

        // Start connecting
        WiFi.begin(ssid.c_str(), pw.c_str());
    }

    // Check if connecting
    int wifiStatus = WiFi.status();
    if (prevWiFiStatus != wifiStatus)
    {
        String msg;
        switch(wifiStatus)
        {
            case WL_CONNECTED: msg =       "Connected ok   "; break;
            case WL_IDLE_STATUS: msg =     "Connecting ... "; break;
            case WL_DISCONNECTED: msg =    "Disconnected   "; break;
            case WL_CONNECT_FAILED: msg =  "Conn failed    "; break;
            case WL_CONNECTION_LOST: msg = "Conn lost      "; break;
            case WL_NO_SHIELD: return;
            default: msg = "Unknown " + String(wifiStatus) + "   "; break;
        }
        Serial.printf("Wifi status changed to %s\n", msg.c_str());
        display.showConnectionState(msg);
        prevWiFiStatus = wifiStatus;

    }

}
