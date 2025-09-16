

#include <FastLED.h>      // Main FastLED library for controlling LEDs

#define CREATE_DEBUG_OUTPUT     // define to create FPS and free RAM debug output in the serial console
//#define USE_RED_GREEN_IDLE_ANIMATION   // define to use the red-green idle animation 
#define PLAY_IDLE_ANIM_NOTES // define to play MIDI notes during idle animation

#define NUMBER_OF_PLAYERS 5 // Number of players (stripes)
#define WIDTH 8             // Number of columns per user
#define HEIGHT 50           // Number of rows in the matrix
#define PLAYER_MAX_YPOS 18
#define BIGWAVE_YPOS 30
#define LEDSTRIPE_COLOR_LAYOUT RGB

#define MODE_RANDOM 0
#define MODE_TEAM   1
#define MODE_CANON  2

#define WAVE_SPEED_LOWER 0.02f    
#define WAVE_SPEED_UPPER 0.012f

#define WAVE_DAMPING_LOWER_TRIGGER 12.0f
#define WAVE_DAMPING_UPPER_TRIGGER 12.0f

#define WAVE_DAMPING_LOWER_RELEASE 6.0f
#define WAVE_DAMPING_UPPER_RELEASE 5.0f

#define WAVE_DAMPING_LOWER_IDLEANIM 7.0f
#define WAVE_DAMPING_UPPER_IDLEANIM 5.0f

#define BLUR_AMOUNT_LOWER 0
#define BLUR_AMOUNT_UPPER 95
#define BLUR_PASSES_LOWER 1
#define BLUR_PASSES_UPPER 1

#define TRIGGER_IMPACT_VALUE 1

#define BIGWAVE_SPEED_LOWER 0.007f
#define BIGWAVE_SPEED_UPPER 0.004f
#define BIGWAVE_DAMPING_LOWER 10.0f
#define BIGWAVE_DAMPING_UPPER 10.5f
#define BIGWAVE_MIDI_CHANNEL 7  

#define NUM_LEDS (WIDTH * HEIGHT * NUMBER_OF_PLAYERS)   // Total number of LEDs
#define NUM_LEDS_PER_PLANE (WIDTH * HEIGHT)   // Total number of LEDs per stripe (multi strip setup)
#define IS_SERPINTINE true              // Whether the LED strip zigzags back and forth (common in matrix layouts)

#define NEOPIXEL_PIN 8      // First data pin for the LED stripes
#define POTI_GND_PIN 25       // ground pin for potentiometers

#define MODE_POTI_PIN       A10   // mode selection potentiometer signal pin
#define VOLUME_POTI_PIN     A12   // volume potentiometer signal pin
#define POTENTIOMETER_NOISE_THRESHOLD 8  // Minimum change in potentiometer value to consider it a valid change
#define POTENTIOMETER_UPDATE_PERIOD 10   // Time in milliseconds between potentiometer reads
#define POTENTIOMENTER_UI_UPDATE_PERIOD 100  // Time in milliseconds between UI updates based on potentiometer changes

#define MAXIMUM_BRIGHTNESS 255    // Maximum brightness for the all leds (0-255)
#define MIDINOTE_VELOCITY 120   // MIDI note velocity for sending notes

#define BIGWAVE_TIME_THRESHOLD 20  // Time in milliseconds to consider two wave triggers as "close enough" for big waves
#define EXTERNAL_TRIGGER_ACTIVE_PERIOD 2000 // Time in milliseconds to override buttons with external triggers (from Serial1)

#define BIGWAVE_MIDINOTE_DURATION 5000 // Duration in milliseconds for big wave effect (fixed duration)
#define USER_ACTIVITY_TIMEOUT 10000 // Time in milliseconds to consider user inactive

#define AUTO_PLAY_TONESCALE_CHANGE_INTERVAL 120 // Time in milliseconds for notes in a tone scale preview
#define MIDI_CHANNEL_FOR_PREVIEW 3 // MIDI channel for automatic tonescale preview notes
#define AUTO_PLAY_PREVIEW_TONES 12 // Number of tones to play in the automatic preview

// Important: super sampling consumes more RAM and CPU! (2X or 4X result in out-of-memory crashes if full 40x50 matrix is used!!
#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_NONE;  // SUPER_SAMPLE_2X or SUPER_SAMPLE_4X to create smoother waves
//#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_2X; 

void wavefx_setup();
void wavefx_loop();

