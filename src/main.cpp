

/*
   Neopixel Kalimba, for Zoom Museum Vienna, 2025
   (c) Michael Strohmann and Chris Veigl

   This firmware is based on the FastLED library and the demo for the FxWave2d effect,
   see: https://github.com/FastLED/FastLED/releases
   
*/

#include <Arduino.h>      // Core Arduino functionality
#include <FastLED.h>      // Main FastLED library for controlling LEDs

#include "wavefx.h"
using namespace fl;        // Use the FastLED namespace for convenience

// #define ONLY_SIGNAL_TRACE_DISPLAY   // define this to just display analog signal traces for testing (no wave effects!)

#ifdef ONLY_SIGNAL_TRACE_DISPLAY
    #include "Averager.h"  // Include the Averager class for smoothing signal traces
    Averager avg50(50);
#endif

void setup() {
    uint32_t timeout_end = millis() + 2000;
    Serial.begin(115200);
    while(!Serial && timeout_end > millis()) {}  // wait until the connection to the PC is established
    delay (1000);
    #ifndef ONLY_SIGNAL_TRACE_DISPLAY
        wavefx_setup();  // Initialize the wave effects and LED strip
        Serial.println("Welcome to the Neopixel Kalimba!");
    #endif

}

void loop() {
    #ifdef ONLY_SIGNAL_TRACE_DISPLAY
        int front=analogRead(A9);
        int back=analogRead(A8);
        int level=front-back;
        Serial.printf("%d,%d\n",  level, avg50.process(level));
        delay(5);
    #else
        wavefx_loop();  // Run the main loop for wave effects and LED updates
    #endif
}
