

#include <FastLED.h>      // Main FastLED library for controlling LEDs

// #define USE_BIG_MATRIX          // define to use a 40x50 led matrix (5 parallel stripes of 8x50 LEDs each)
#define CREATE_DEBUG_OUTPUT     // define to create FPS and free RAM debug output in the serial console

#define NUMBER_OF_PLAYERS 5 // Number of players (stripes)

#ifdef USE_BIG_MATRIX
    #define WIDTH 8             // Number of columns in the matrix  
    #define HEIGHT 50           // Number of rows in the matrix
#else
    #define WIDTH 8             // Number of columns in the matrix  
    #define HEIGHT 32           // Number of rows in the matrix
#endif


#define NUM_LEDS (WIDTH * HEIGHT * NUMBER_OF_PLAYERS)   // Total number of LEDs
#define NUM_LEDS_PER_PLAYER (WIDTH) * (HEIGHT)   // Total number of LEDs per stripe (multi strip setup)
#define IS_SERPINTINE true              // Whether the LED strip zigzags back and forth (common in matrix layouts)

#define NEOPIXEL_PIN 8      // First data pin for the LED stripes
#define USER_POTI_GND_PIN 25       // user potentiometer ground pin
#define USER_POTI_SIGNAL_PIN A10   // user potentiometer signal pin

#define MAXIMUM_BRIGHTNESS 150   // Maximum brightness for the LED strip (0-255)
#define MIDINOTE_VELOCITY 120   // MIDI note velocity for sending notes

#define BIGWAVE_TIME_THRESHOLD 20  // Time in milliseconds to consider two wave triggers as "close enough" for big waves
#define EXTERNAL_TRIGGER_ACTIVE_PERIOD 2000 // Time in milliseconds to override buttons with external triggers (from Serial1)

#define BIGWAVE_MIDINOTE_DURATION 5000 // Duration in milliseconds for big wave effect (fixed duration)
#define USER_ACTIVITY_TIMEOUT 30000 // Time in milliseconds to consider user inactive

// Important: super sampling consumes more RAM and CPU! (2X or 4X result in out-of-memory crashes if full 40x50 matrix is used!!
#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_NONE;  // SUPER_SAMPLE_2X or SUPER_SAMPLE_4X to create smoother waves
//#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_2X; 


void wavefx_setup();
void wavefx_loop();

