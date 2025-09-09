

#include <FastLED.h>      // Main FastLED library for controlling LEDs

// #define USE_BIG_MATRIX          // define to use a 40x50 led matrix (5 parallel stripes of 8x50 LEDs each)
#define CREATE_DEBUG_OUTPUT     // define to create FPS and free RAM debug output in the serial console
//#define USE_RED_GREEN_IDLE_ANIMATION   // define to use the red-green idle animation 
#define PLAY_IDLE_ANIM_NOTES // define to play MIDI notes during idle animation


// specific parameters for small led-matrix or big matrix (led-curtain)
#ifdef USE_BIG_MATRIX
    #define ENABLE_PITCHBEND  0    // 1 enables pitching via horizontal joystick movement, 0 disables it
    #define ENABLE_TONE_WANDERING 0    // 1 enables tone wandering via vertical joystick movement, 0 disables it
    #define ENABLE_BIGWAVE  1    // 1 enables big wave effect, 0 disables it

    #define NUMBER_OF_PLAYERS 5 // Number of players (stripes)
    #define PLANES_PER_PLAYER 1
    #define WIDTH 8             // Number of columns per user
    #define HEIGHT 50           // Number of rows in the matrix
    #define PLAYER_MAX_YPOS 18
    #define BIGWAVE_YPOS 30
	#define LEDSTRIPE_COLOR_LAYOUT RGB

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

    #define JOYSTICK_MODE 0
    #define TRIGGER_IMPACT_VALUE 1
#else
    #define ENABLE_PITCHBEND  1    // 1 enables pitching via horizontal joystick movement, 0 disables it
    #define ENABLE_TONE_WANDERING 0    // 1 enables tone wandering via vertical joystick movement, 0 disables it
    #define ENABLE_BIGWAVE  0    // 1 enables big wave effect, 0 disables it

    #define NUMBER_OF_PLAYERS 3 // Number of players (stripes)
    #define PLANES_PER_PLAYER 2
    #define WIDTH 8
    #define HEIGHT 32
    #define PLAYER_MAX_YPOS 30
    #define BIGWAVE_YPOS 16
	#define LEDSTRIPE_COLOR_LAYOUT GRB

    #define WAVE_SPEED_LOWER 0.03f
	#define WAVE_SPEED_UPPER 0.02f

    #define WAVE_DAMPING_LOWER_TRIGGER 11.0f
    #define WAVE_DAMPING_UPPER_TRIGGER 9.0f

    #define WAVE_DAMPING_LOWER_RELEASE 7.0f
	#define WAVE_DAMPING_UPPER_RELEASE 6.0f

    #define WAVE_DAMPING_LOWER_IDLEANIM 9.0f
    #define WAVE_DAMPING_UPPER_IDLEANIM 7.0f

    #define BLUR_AMOUNT_LOWER 50
    #define BLUR_AMOUNT_UPPER 50
    #define BLUR_PASSES_LOWER 2
    #define BLUR_PASSES_UPPER 2

    #define JOYSTICK_MODE 1
    #define TRIGGER_IMPACT_VALUE 0.15f
#endif

#define BIGWAVE_SPEED_LOWER 0.007f
#define BIGWAVE_SPEED_UPPER 0.004f
#define BIGWAVE_DAMPING_LOWER 10.0f
#define BIGWAVE_DAMPING_UPPER 10.5f

#define NUM_LEDS (WIDTH * HEIGHT * NUMBER_OF_PLAYERS * PLANES_PER_PLAYER)   // Total number of LEDs
#define NUM_LEDS_PER_PLANE (WIDTH) * (HEIGHT)   // Total number of LEDs per stripe (multi strip setup)
#define IS_SERPINTINE true              // Whether the LED strip zigzags back and forth (common in matrix layouts)

#define NEOPIXEL_PIN 8      // First data pin for the LED stripes
#define USER_POTI_GND_PIN 25       // user potentiometer ground pin
#define USER_POTI_SIGNAL_PIN A10   // user potentiometer signal pin
#define SWITCH_CHANNEL_BUTTON_PIN 30 // first pin for the 3 user buttons to switch midi channels

#define JOYSTICK_MOVEMENT_IMPACT 0.005

#define MAXIMUM_BRIGHTNESS 255    // Maximum brightness for the all leds (0-255)
#define MIDINOTE_VELOCITY 120   // MIDI note velocity for sending notes

#define BIGWAVE_TIME_THRESHOLD 20  // Time in milliseconds to consider two wave triggers as "close enough" for big waves
#define EXTERNAL_TRIGGER_ACTIVE_PERIOD 2000 // Time in milliseconds to override buttons with external triggers (from Serial1)

#define BIGWAVE_MIDINOTE_DURATION 5000 // Duration in milliseconds for big wave effect (fixed duration)
#define USER_ACTIVITY_TIMEOUT 10000 // Time in milliseconds to consider user inactive

// Important: super sampling consumes more RAM and CPU! (2X or 4X result in out-of-memory crashes if full 40x50 matrix is used!!
#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_NONE;  // SUPER_SAMPLE_2X or SUPER_SAMPLE_4X to create smoother waves
//#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_2X; 


void wavefx_setup();
void wavefx_loop();

