/*  
   Neopixel Kalimba, for Zoom Museum Vienna, 2025
   (c) Michael Strohmann and Chris Veigl

    The Neopixel Kalimba uses two layered WaveFx effects (upper and lower layer) per player, 
    The number of players is configurable, each player can use different colors palettes and wave parameters,
    which are blended together to create complex visual effects.
    Waves can be triggered by buttons or piezo sensors, and the wave parameters can be adjusted using analog inputs (e.g., an analog joystick).
    This firmware uses the FastLED library for LED control and supports MIDI output for controlling MIDI devices (e.g. the ZynAddSubFX synthesizer).
    It is designed to run on a Teensy 4.1 and supports Neopixel matrices up to 2000 pixels.
*/

#include <Arduino.h>      // Core Arduino functionality
#include <FastLED.h>      // Main FastLED library for controlling LEDs

#include "fl/math_macros.h"  // Math helper functions and macros
#include "fl/time_alpha.h"   // Time-based alpha/transition effects
#include "fx/2d/blend.h"     // 2D blending effects between layers
#include "fx/2d/wave.h"      // 2D wave simulation
#include "colors&tonescales.h" // Color definitions and tone scales
#include "wavefx.h"  // Header file for this sketch

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

int freeram() {
  return (char *)&_heap_end - __brkval;
}

using namespace fl;        // Use the FastLED namespace for convenience

// Array to hold all LED color values - one CRGB struct per LED
CRGB leds[NUM_LEDS];


// Create mappings between 1D array positions and 2D x,y coordinates
XYMap xyMap(WIDTH , HEIGHT, IS_SERPINTINE);  // For the actual LED output (may be serpentine)
XYMap xyRect(WIDTH , HEIGHT, false);         // For the wave simulation (always rectangular grid)

// Create a blender that will combine the two wave layers
Blend2d fxBlend(xyMap);

// Create default configuration for the wave layers
WaveFx::Args CreateDefWaveArgs() {
    WaveFx::Args out;
    out.factor = SUPER_SAMPLE_MODE;  // Use the defined super sampling mode
    out.half_duplex = true;                    // Only positive waves (no negative values)
    out.auto_updates = true;                   // Automatically update the simulation each frame
    out.x_cyclical = true;                     // Enable horizontal wrapping (cylindrical effect)
    return out;
}

// Wave effects for bigWave
WaveFx bigWaveLower(xyRect, CreateDefWaveArgs());
WaveFx bigWaveUpper(xyRect, CreateDefWaveArgs());            


// Create a player data structure to hold wave layers and user interaction data
struct PlayerData {
    WaveFx waveLower;
    WaveFx waveUpper;
    TimeRamp bigWaveTransition;  // Transition for the big wave effect

 
    int playerId;
    int analogPin;  // Pin for analog input (e.g., potentiometer)
    int trigger1Pin;  // Pin for the button to trigger waves
    int trigger2Pin;  // Pin for the button to trigger big waves
    int trigger1Active;
    int trigger2Active;
    int trigger1Note;
    int trigger2Note;
    int triggerBigWaveActive;
    int * tonescale = nullptr;  // Pointer to the current tone scale (if needed)
    int tonescaleSize = 0;
    
    // Constructor
    PlayerData(const XYMap& xyRect, const WaveFx::Args& argsLower, const WaveFx::Args& argsUpper, int id, int pinAnalog, int pin, int pinBig )
        : waveLower(xyRect, argsLower),
          waveUpper(xyRect, argsUpper),
          bigWaveTransition (70, 0, 0),  
          playerId(id),
          analogPin(pinAnalog),
          trigger1Pin(pin),
          trigger2Pin(pinBig),
          trigger1Active(0),
          trigger2Active(0),
          trigger1Note(0),
          trigger2Note(0),
          triggerBigWaveActive(0) 
    {}
};

PlayerData playerArray[NUMBER_OF_PLAYERS] = {
    { xyRect, CreateDefWaveArgs(), CreateDefWaveArgs(), 0, A9,  22, 21},
    { xyRect, CreateDefWaveArgs(), CreateDefWaveArgs(), 1, A6,  19, 18},
    { xyRect, CreateDefWaveArgs(), CreateDefWaveArgs(), 2, A3,  16, 15},
    { xyRect, CreateDefWaveArgs(), CreateDefWaveArgs(), 3, A0,  41, 40},
    { xyRect, CreateDefWaveArgs(), CreateDefWaveArgs(), 4, A15, 38, 37 }
};


// Create a ripple effect at a random position within the central area of the display
void triggerWave(int pos, PlayerData * player) {
    // Define a margin percentage to keep ripples away from the edges
    float perc = .15f;
    
    // Calculate the boundaries for the ripple (15% from each edge)
    uint8_t min_x = perc * WIDTH;          // Left boundary
    uint8_t max_x = (1 - perc) * WIDTH;    // Right boundary
    uint8_t min_y = perc * HEIGHT;         // Top boundary
    uint8_t max_y = (1 - perc) * HEIGHT;   // Bottom boundary
    
    // Generate a random position within these boundaries
    int x = random(min_x, max_x);
    int y = pos==-1 ? random(min_y, max_y) : pos;
    
    #ifdef TEST_MODE
      // In test mode, use a fixed position for small led matrix
      int xOffset =0;
    #else
      int xOffset = playerId * WIDTH;  // Offset for the player ID to separate wave layers
    #endif


    // Set a wave peak at this position in both wave layers
    // The value 1.0 represents the maximum height of the wave
    player->waveLower.setf(x + xOffset, y, 1);  // Create ripple in lower layer
    player->waveUpper.setf(x + xOffset, y, 1);  // Create ripple in upper layer
    // Serial.printf("Triggering ripple at (%d, %d)\n", x, y);
}

// Create a fancy cross-shaped effect that expands from the center
void applyBigWave(uint32_t now, bool button_active, PlayerData * player) {
    // If the button is active, start/restart the animation
    if (button_active) {
        player->bigWaveTransition.trigger(now, 70, 0, 0);
    }

    // If the animation isn't currently active, exit early
    if (!player->bigWaveTransition.isActive(now)) {
        // no need to draw
        return;
    }
    
    #ifdef TEST_MODE
      // In test mode, use a fixed position for small led matrix
      int xOffset =0;
    #else
      int xOffset = playerId * WIDTH;  // Offset for the player ID to separate wave layers
    #endif


    // Find the center of the display
    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    
    // Calculate the maximum distance from center (half the width)
    int amount = WIDTH / 4;   // /2
    
    // Calculate the start and end coordinates for the cross
    int start_x = mid_x - amount;  // Leftmost point
    int end_x = mid_x + amount;    // Rightmost point
    int start_y = mid_y - amount;  // Topmost point
    int end_y = mid_y + amount;    // Bottommost point
    
    // Get the current animation progress (0-255)
    int curr_alpha = player->bigWaveTransition.update8(now);

    // Map the animation progress to the four points of the expanding cross
    // As curr_alpha increases from 0 to 255, these points move from center to edges
    int left_x = map(curr_alpha, 0, 255, mid_x, start_x);  // Center to left
    int down_y = map(curr_alpha, 0, 255, mid_y, start_y);  // Center to top
    int right_x = map(curr_alpha, 0, 255, mid_x, end_x);   // Center to right
    int up_y = map(curr_alpha, 0, 255, mid_y, end_y);      // Center to bottom

    // Convert the 0-255 alpha to 0.0-1.0 range
    float curr_alpha_f = curr_alpha / 255.0f;

    // Calculate the wave height value - starts high and decreases as animation progresses
    // This makes the waves stronger at the beginning of the animation
    float valuef = (1.0f - curr_alpha_f) * 0.5f; // intensity

    // Calculate the width of the cross lines
    int span = 5; //  0.5 * WIDTH;  // fancyParticleSpan.value()

    // Add wave energy along the four expanding lines of the cross
    // Each line is a horizontal or vertical span of pixels
    
    // Left-moving horizontal line
    for (int x = left_x - span; x < left_x + span; x++) {
        bigWaveLower.addf(x + xOffset, mid_y, valuef);  // Add to lower layer
        bigWaveUpper.addf(x + xOffset, mid_y, valuef);  // Add to upper layer
    }

    // Right-moving horizontal line
    for (int x = right_x - span; x < right_x + span; x++) {
        bigWaveLower.addf(x + xOffset, mid_y, valuef);
        bigWaveUpper.addf(x + xOffset, mid_y, valuef);
    }

    // Downward-moving vertical line
    for (int y = down_y - span; y < down_y + span; y++) {
        bigWaveLower.addf(mid_x + xOffset, y, valuef);
        bigWaveUpper.addf(mid_x + xOffset, y, valuef);
    }

    // Upward-moving vertical line
    for (int y = up_y - span; y < up_y + span; y++) {
        bigWaveLower.addf(mid_x + xOffset, y, valuef);
        bigWaveUpper.addf(mid_x + xOffset, y, valuef);
    }
}

void setWaveParameters( WaveFx & waveLower, float speed, float dampening) {
    // Set the speed and dampening for one wave layer
    waveLower.setSpeed(speed);
    waveLower.setDampening(dampening);
}

void processPlayers(PlayerData * player) {

    int trigger1State = digitalRead(player->trigger1Pin);
    int trigger2State = digitalRead(player->trigger2Pin);
    int noteControlStickValue = analogRead(player->analogPin);

    if ((trigger1State == LOW) && (player->trigger1Active == 0)) {
        player->trigger1Active = 1;
        if (JOYSTICK_MODE == 0) noteControlStickValue = random (0,512);  // if no joystick attached, use random value in bottom half!
        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, 0.005f, 12.0f);        // (speed, dampening)
        setWaveParameters(player->waveUpper, 0.003f, 12.0f);   
        triggerWave(map (noteControlStickValue, 0, 1023, 2, HEIGHT-2), player);  // create a wave at the determined position

        // Map the analog input to a MIDI note in the defined scale
        player->trigger1Note = player->tonescaleSize > 0 
            ? 60 + player->tonescale[map (noteControlStickValue, 0, 1023, 0, player->tonescaleSize - 1)]  // Map to a note in the scale
            : map (noteControlStickValue, 0, 1023, 10, 90);  // take all midi tones in range 10-90 if no scale is defined

        Serial.printf("Triggering player %d bottom half wave at analog value %d, mapped to note %d\n", player->playerId, noteControlStickValue, player->trigger1Note);
        usbMIDI.sendNoteOn(player->trigger1Note, MIDINOTE_VELOCITY, player->playerId+1);       // MIDI channel = player id + 1
    }   
    else if ((trigger1State == HIGH)  && (player->trigger1Active == 1)) {
        player->trigger1Active = 0;
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, 0.005f, 10.0f);
        setWaveParameters(player->waveUpper, 0.003f, 8.0f);
        usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->playerId+1); 
    } 

    if ((trigger2State == LOW) && (player->trigger2Active == 0)) {  
        player->trigger2Active = 1;
        if (JOYSTICK_MODE == 0) noteControlStickValue = random (512,1023);  // if no joystick attached, use random value in top half!
        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, 0.005f, 12.0f);        // (speed, dampening)
        setWaveParameters(player->waveUpper, 0.003f, 12.0f);   
        triggerWave(map (noteControlStickValue, 0, 1023, 2, HEIGHT-2), player);  // create a wave at the determined position

        // Map the analog input to a MIDI note in the defined scale
        player->trigger2Note = player->tonescaleSize > 0 
            ? 60 + player->tonescale[map (noteControlStickValue, 0, 1023, 0, player->tonescaleSize - 1)]  // Map to a note in the scale
            : map (noteControlStickValue, 0, 1023, 10, 90);  // take all midi tones in range 10-90 if no scale is defined

        Serial.printf("Triggering Player %d top half wave at analog value %d, mapped to note %d\n", player->playerId, noteControlStickValue, player->trigger2Note);
        usbMIDI.sendNoteOn(player->trigger2Note, MIDINOTE_VELOCITY, player->playerId+1);  // MIDI channel = player id + 1
    }
    else if ((trigger2State == HIGH) && (player->trigger2Active == 1)) {
        player->trigger2Active = 0;  // Reset fancy button state
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, 0.005f, 10.0f);
        setWaveParameters(player->waveUpper, 0.003f, 8.0f);
        usbMIDI.sendNoteOff(player->trigger2Note, MIDINOTE_VELOCITY, player->playerId+1); 
    }

    /* 

    trigger big wave effect
        bigNote = 50;  // Fixed MIDI note for big wave
        setWaveParameters(bigWaveLower, 0.007f, 10.0f);
        setWaveParameters(bigWaveUpper, 0.004f, 10.0f);
        usbMIDI.sendNoteOn(bigNote, MIDINOTE_VELOCITY, 7);  // Send MIDI note for big wave effect, channel 7

        setWaveParameters(bigWaveLower, 0.020f, 8.0f);
        setWaveParameters(bigWaveUpper, 0.018f, 8.0f);
        usbMIDI.sendNoteOff(bigNote, MIDINOTE_VELOCITY, 7);  // Send MIDI note for big wave effect, channel 7

        applyBigWave(millis(), triggerBigWaveActive, player);  // Trigger the bigwave effect
    */
}

// Setup function to initialize the wave effects and LED strip
void wavefx_setup() {

    pinMode (USER_BUTTON_PIN, INPUT_PULLUP);
    Serial.print("Initial Free Ram = "); Serial.println(freeram());

    auto screenmap = xyMap.toScreenMap();
    // screenmap.setDiameter(2);  // Set the size of the LEDs in the visualization

    // Initialize the LED strip (setScreenMap connects our 2D coordinate system to the 1D LED array)
    // TBD: verify correct format for Teensy4.1 5 parallel stripes
    CLEDController& c1 = FastLED.addLeds<WS2812, 8, GRB>(leds, NUM_LEDS_PER_PLAYER);
    /*  CLEDController& c2 = FastLED.addLeds<WS2812, 9, GRB>(&leds[NUM_LEDS_PER_PLAYER*1], NUM_LEDS_PER_PLAYER).setScreenMap(screenmap);
        CLEDController& c3 = FastLED.addLeds<WS2812, 10, GRB>(&leds[NUM_LEDS_PER_PLAYER*2], NUM_LEDS_PER_PLAYER).setScreenMap(screenmap);
        CLEDController& c4 = FastLED.addLeds<WS2812, 11, GRB>(&leds[NUM_LEDS_PER_PLAYER*3], NUM_LEDS_PER_PLAYER).setScreenMap(screenmap);
        CLEDController& c5 = FastLED.addLeds<WS2812, 12, GRB>(&leds[NUM_LEDS_PER_PLAYER*4], NUM_LEDS_PER_PLAYER).setScreenMap(screenmap);
    */

    FastLED.setBrightness(OVERALL_BRIGHTNESS);  // Set the overall brightness (0-255)

    // Initialize the color palettes for the wave layers
    WaveCrgbMapPtr palYellowRed, palDarkGreen, palDarkBlue, palYellowWhite, palPurpleWhite;  // Color palettes for the wave layers
    palYellowRed = fl::make_shared<WaveCrgbGradientMap>(yellowRedGradientPal);
    palPurpleWhite = fl::make_shared<WaveCrgbGradientMap>(purpleWhiteGradientPal);
    palDarkBlue = fl::make_shared<WaveCrgbGradientMap>(darkBlueGradientPal);
    palDarkGreen = fl::make_shared<WaveCrgbGradientMap>(darkGreenGradientPal);
    palYellowWhite = fl::make_shared<WaveCrgbGradientMap>(yellowWhiteGradientPal);

    // Create parameter structures for each wave layer's blur settings
    Blend2dParams lower_params = {
        .blur_amount = 0,            // Blur amount for lower layer
        .blur_passes = 1,            // Blur passes for lower layer
    };

    Blend2dParams upper_params = {
        .blur_amount = 95,           // Blur amount for upper layer
        .blur_passes = 1,            // Blur passes for upper layer
    };

    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {

        PlayerData& p = playerArray[i]; // Use a reference for clarity and efficiency
        p.tonescaleSize=0;
        pinMode (p.trigger1Pin, INPUT_PULLUP);    // Set button pin as input with pull-up resistor
        pinMode (p.trigger2Pin, INPUT_PULLUP); // Set big button pin as input with pull-up resistor

        p.tonescale = (int *)tonescalePentatonicMajor;
        p.tonescaleSize = sizeof(tonescalePentatonicMajor) / sizeof(tonescalePentatonicMajor[0]);

        p.waveLower.setCrgbMap(palDarkBlue);
        p.waveLower.setEasingMode(U8EasingFunction::WAVE_U8_MODE_LINEAR);
        p.waveUpper.setCrgbMap(palPurpleWhite);
        p.waveUpper.setEasingMode(U8EasingFunction::WAVE_U8_MODE_LINEAR);

        // Add wave layers to the blender (order matters - lower layer is added first (background))
        fxBlend.add(p.waveLower);
        fxBlend.add(p.waveUpper);

        // Apply the layer-specific blur parameters
        fxBlend.setParams(p.waveLower, lower_params);
        fxBlend.setParams(p.waveUpper, upper_params);
    }

    bigWaveLower.setCrgbMap(palYellowRed);
    bigWaveLower.setEasingMode(U8EasingFunction::WAVE_U8_MODE_LINEAR);
    bigWaveUpper.setCrgbMap(palYellowRed);
    bigWaveUpper.setEasingMode(U8EasingFunction::WAVE_U8_MODE_LINEAR);

    fxBlend.add(bigWaveLower);  // Add the big wave layers to the blender
    fxBlend.add(bigWaveUpper);
    fxBlend.setParams(bigWaveLower, lower_params);
    fxBlend.setParams(bigWaveUpper, upper_params);

    // Now set player-specific color palettes and tone scales
    playerArray[1].waveLower.setCrgbMap(palDarkGreen);
    playerArray[1].waveUpper.setCrgbMap(palYellowWhite);


    // Apply global blur settings to the blender
    fxBlend.setGlobalBlurAmount(0);       // Overall blur strength
    fxBlend.setGlobalBlurPasses(1);       // Number of blur passes
}


void wavefx_loop() {
    uint32_t now = millis();

    // Apply current settings and get button states for all players
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
        processPlayers(&playerArray[i]);
    }   

    Fx::DrawContext ctx(now, leds); // Create a drawing context with the current time and LED array
    fxBlend.draw(ctx);      // Draw the blended result of both wave layers to the LED array
    FastLED.show();         // Send the color data to the actual LEDs

    static int frameCount = 0;  // Frame counter for performance monitoring
    static int frameTime = 0;   // Time taken for the last frame
    frameCount++;

    // If debug output is enabled, print the frame rate and free RAM every second
    #ifdef CREATE_DEBUG_OUTPUT
        if (millis() - frameTime >= 1000) {
            // Every second, print the frame rate
            Serial.printf("FPS: %d, Free Ram = %d, PixelPin=%d\n", frameCount, freeram(), NEOPIXEL_PIN);
            frameCount = 0;  // Reset frame counter
            frameTime = millis();  // Update last frame time
        }
    #endif
}


// FastLED.addLeds<NUM_PLAYERS, WS2811, 10>(leds,NUM_LEDS_PER_PLAYER).setScreenMap(screenmap);
/*
    for (int i=0; i< NUM_PLAYERS; i++) {
        switch (i) {
            case 0: FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN+0>(leds, NUM_LEDS_PER_PLAYER*i, NUM_LEDS_PER_PLAYER).setScreenMap(screenmap); break;
            case 1: FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN+1>(leds, NUM_LEDS_PER_PLAYER*i, NUM_LEDS_PER_PLAYER).setScreenMap(screenmap); break;
            case 2: FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN+2>(leds, NUM_LEDS_PER_PLAYER*i, NUM_LEDS_PER_PLAYER).setScreenMap(screenmap); break;
            case 3: FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN+3>(leds, NUM_LEDS_PER_PLAYER*i, NUM_LEDS_PER_PLAYER).setScreenMap(screenmap); break;
            case 4: FastLED.addLeds<NEOPIXEL, NEOPIXEL_PIN+4>(leds, NUM_LEDS_PER_PLAYER*i, NUM_LEDS_PER_PLAYER).setScreenMap(screenmap); break;
        }
    }

*/

/*
    static int xPos=0, yPos=0;
    leds[xyMap(xPos, yPos) ] = CRGB::Red;
    FastLED.show();
    xPos++;
    if (xPos >= WIDTH) {
        xPos = 0;  // Reset x position when reaching the end of the row
        yPos++;
        if (yPos >= HEIGHT) {
            yPos = 0;  // Reset y position when reaching the end of the column
        }
    }
    delay(100);  // Delay to control the speed of the animation
*/

/*
     instrument = (instrument + 1) % 10; // Cycle through instruments 0-9
     usbMIDI.sendProgramChange(instrument, 1);
     usbMIDI.sendControlChange(0, instrument, 1);  // Bank Select MSB = 1
     usbMIDI.sendControlChange(32, 0, 1); // Bank Select LSB = 0
*/
