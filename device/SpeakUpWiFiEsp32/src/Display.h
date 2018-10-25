// Display handler for WiFi credentials over sound
// Rob Dobson 2018

// Heltec 0.96" OLED ESP32 Module
// To run without this display just comment out the following line
#define HELTEC_0_96_IN_OLED_ESP32 1

#ifdef HELTEC_0_96_IN_OLED_ESP32
// Heltec display driver is U8g2 https://github.com/olikraus/u8g2
// The line ... lib_deps=U8g2 ... needs to be in Platformio.ini file
#include <U8x8lib.h>
// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
#endif

class Display
{
public:
    Display()
    {
#ifdef HELTEC_0_96_IN_OLED_ESP32
        u8x8.begin();
        u8x8.setFont(u8x8_font_chroma48medium8_r);
#endif
    }

    void welcome(int adc1Channel)
    {
        int pin = (adc1Channel < 4) ? adc1Channel + 36 : adc1Channel - 4 + 32;
        String dispMsg = "Mic on pin " + String(pin);
#ifdef HELTEC_0_96_IN_OLED_ESP32
        u8x8.drawString(0, 0, dispMsg.c_str());
        u8x8.drawString(0, 1, "Listening ...");
#endif
    }    

    void showSSID(String& ssid)
    {
#ifdef HELTEC_0_96_IN_OLED_ESP32
        u8x8.drawString(0, 3, "Connecting to");
        u8x8.drawString(0, 4, ssid.c_str());
#endif
    }    

    void showConnectionState(String& msg)
    {
#ifdef HELTEC_0_96_IN_OLED_ESP32
        u8x8.drawString(0, 6, msg.c_str());
#endif
    }

};
