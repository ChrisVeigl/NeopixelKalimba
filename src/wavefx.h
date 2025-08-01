

#include <FastLED.h>      // Main FastLED library for controlling LEDs

// Define the dimensions of our LED matrix
#define NEOPIXEL_PIN 8      // First data pin for the LED stripes
#define HEIGHT 32           // Number of rows in the matrix
#define WIDTH 8             // Number of columns in the matrix  
#define NUMBER_OF_PLAYERS 5 // Number of players (stripes)

#define OVERALL_BRIGHTNESS 50   // Overall brightness for the LED strip (0-255)
#define MIDINOTE_VELOCITY 120   // MIDI note velocity for sending notes

#define NUM_LEDS (WIDTH * HEIGHT * NUMBER_OF_PLAYERS)   // Total number of LEDs
#define NUM_LEDS_PER_PLAYER (WIDTH) * (HEIGHT)   // Total number of LEDs per stripe (multi strip setup)
#define IS_SERPINTINE true              // Whether the LED strip zigzags back and forth (common in matrix layouts)

#define USER_BUTTON_PIN 33      // user button pin (generic functionality)

#define TEST_MODE               // define to test all players with a single 8x40 led matrix
#define CREATE_DEBUG_OUTPUT     // define to create FPS and free RAM debug output in the serial console

#define JOYSTICK_MODE 0        // 0 = random, 1 = analog stick

// Important: super sampling consumes more RAM and CPU! (2X or 4X result in out-of-memory crashes if full 40x50 matrix is used!!
#define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_NONE;  // SUPER_SAMPLE_2X or SUPER_SAMPLE_4X to create smoother waves
// #define SUPER_SAMPLE_MODE SuperSample::SUPER_SAMPLE_2X; 
                                                  
void wavefx_setup();
void wavefx_loop();

