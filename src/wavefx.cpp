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

#include "wavefx.h"  // Header file for this sketch
#include "colors&tonescales.h" // Color definitions and tone scales
#include "pixelmap.h"  // Pixel mapping for the LED matrix
#include "utils.h"  // Utility functions (e.g., random number generation)

using namespace fl;        // Use the FastLED namespace for convenience

// Array to hold all LED color values - one CRGB struct per LED
CRGB leds[NUM_LEDS];

uint8_t trigger1Flags, trigger2Flags = 0;  // Flags to indicate if a trigger event has occurred (received from Serial1)
uint32_t trigger1FlagsUpdateTime = 0, trigger2FlagsUpdateTime = 0;  // Timestamps for last trigger updates (from Serial1)

// Create mappings between 1D array positions and 2D x,y coordinates
#ifdef USE_BIG_MATRIX
    XYMap xyMap = XYMap::constructWithLookUpTable(WIDTH*NUMBER_OF_PLAYERS, HEIGHT, XYTable, 0);  // For the actual LED output (may be serpentine)
    XYMap xyRect(WIDTH * NUMBER_OF_PLAYERS, HEIGHT, false);         // For the wave simulation (always rectangular grid)
#else
   // Custom mapping function: maps (x, y) to a zigzag (serpentine) linear index
    uint16_t myXYMapping(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
        uint16_t col = x >> 3;
        uint16_t add = 256 * col;
        uint16_t x_ind = x & 0x07;

        if (y%2) x_ind=7-x_ind;
        uint16_t mapped = y * 8 + x_ind + add;

        return mapped;
    }

    XYMap xyMap = XYMap::constructWithUserFunction(WIDTH* NUMBER_OF_PLAYERS * PLANES_PER_PLAYER, HEIGHT, myXYMapping);
    XYMap xyRect(WIDTH * NUMBER_OF_PLAYERS * PLANES_PER_PLAYER, HEIGHT, false);         // For the wave simulation (always rectangular grid)



#endif

// Create a blender that will combine the wave effecsts of all players
Blend2d fxBlend(xyRect);

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
WaveFx bigWaveLower(xyMap, CreateDefWaveArgs());
WaveFx bigWaveUpper(xyMap, CreateDefWaveArgs());     
int bigwaveNote = 0;  // MIDI note for big wave effect, will be set later based on player tone scale
uint32_t bigWaveRunTime = 0;  
int bigWaveNoteIndex = 0;  // Index for the "travelling" big wave note in the tone scale
uint32_t lastUserActivity=0;  // Timestamp of the last user interaction (button press, wave trigger, etc.)
int idleAnimNote = 0;  // MIDI note for idle animation, will be set later based on player tone scale


// Create a player data structure to hold wave layers and user interaction data
struct PlayerData {
    WaveFx waveLower;
    WaveFx waveUpper;

    int playerId;
    int midiChannel;
    int analogPin;  // Pin for analog input (e.g., potentiometer)
    int trigger1Pin;  // Pin for the button to trigger waves
    int trigger2Pin;  // Pin for the button to trigger big waves
    int trigger1Active;
    int trigger2Active;
    int trigger1Note;
    int trigger2Note;
    TimeRamp bigWaveTransition;
    int * tonescale = nullptr;  // Pointer to the current tone scale (if needed)
    int tonescaleSize = 0;
    int joystickMode = JOYSTICK_MODE;  // 0 = random, 1 = analog stick (used to determine how to interpret the analog input)
    uint32_t trigger1Timestamp = 0;  // Timestamp for the last trigger1 event
    uint32_t trigger2Timestamp = 0;  // Timestamp for the last trigger2 event

    // Constructor
    PlayerData(const XYMap& xyMap, const WaveFx::Args& argsLower, const WaveFx::Args& argsUpper, int id, int pinAnalog, int pinT1, int pinT2 )
        : waveLower(xyMap, argsLower),
          waveUpper(xyMap, argsUpper),
          playerId(id),
          midiChannel(id+1),
          analogPin(pinAnalog),
          trigger1Pin(pinT1),
          trigger2Pin(pinT2),
          trigger1Active(0),
          trigger2Active(0),
          trigger1Note(0),
          trigger2Note(0),
          bigWaveTransition(70, 0, 0)
    {}
};

#ifdef USE_BIG_MATRIX
PlayerData playerArray[NUMBER_OF_PLAYERS] = {
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 0, A9,  22, 21},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 1, A6,  19, 18},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 2, A3,  16, 15},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 3, A0,  41, 40},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 4, A15, 38, 37 }
};
#else 
PlayerData playerArray[NUMBER_OF_PLAYERS] = {
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 0, A9,  22, A7},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 1, A6,  19, A4},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 2, A3,  16, A1},
};
#endif

void setWaveParameters( WaveFx & waveLower, float speed, float dampening) {
    // Set the speed and dampening for one wave layer
    waveLower.setSpeed(speed);
    waveLower.setDampening(dampening);
}

void playIdleAnimation() {
    static uint32_t lastUpdateTime = 0;  // Timestamp of the last update

    #ifdef USE_RED_GREEN_IDLE_ANIMATION
        static int xPos=0, yPos=0;
        static uint8_t red = 255, green = 0;  // RGB color components
        if (millis() - lastUpdateTime > 10) {  // Update every 10 ms
            lastUpdateTime = millis();  // Update the timestamp
            
            //colorIndex = (colorIndex + 1) % 500;
            //int c = colorIndex < 250 ? colorIndex : (500 - colorIndex);  // Create a gradient effect
            leds[xyMap(xPos, yPos)] = CRGB(red, green, 0);
            xPos++;
            if (xPos >= WIDTH * NUMBER_OF_PLAYERS) {
                xPos = 0;  // Reset x position when reaching the end of the row
                yPos++;
                if (red) {red=0;green=255;} 
                else {red=255;green=0;} // Switch to green color
                if (yPos >= HEIGHT) {
                    yPos = 0;  // Reset y position when reaching the end of the column
                    
                }
            }
        }
    #else
        static float xPos=0.0f, yPos=0.0f;
        static int animCounter=0, playerId = 0, duration=100;  
        static float xSpeed = 0.2f, ySpeed = 0.1f,impact=0.03f;   
        if (millis() - lastUpdateTime > 10) {  // Update every 10 ms
            lastUpdateTime = millis();  // Update the timestamp
            animCounter++;
            if (animCounter > duration *2) {
                playerId = random (0,NUMBER_OF_PLAYERS);
                xSpeed= randomFloat(-0.10f, 0.10f);  // Random horizontal speed
                ySpeed= randomFloat(0.05f, 0.10f); 
                yPos= randomFloat(0.0f, HEIGHT/3);  // Random vertical position
                impact= randomFloat(0.02f, 0.05f);  // Random impact strength
                duration=random(100,500);
                animCounter=0;
                setWaveParameters(playerArray[playerId].waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_IDLEANIM);
                setWaveParameters(playerArray[playerId].waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_IDLEANIM);

                #ifdef PLAY_IDLE_ANIM_NOTES
                idleAnimNote = 60 + playerArray[playerId].tonescale [random(0,7)];
                usbMIDI.sendNoteOn(idleAnimNote, MIDINOTE_VELOCITY, 8);  // Send MIDI note for idle animation
                #endif
            }

            if (animCounter > 0 && animCounter < duration ) {
                xPos += xSpeed; if (xPos >= WIDTH*PLANES_PER_PLAYER*NUMBER_OF_PLAYERS) xPos = 0; if (xPos < 0) xPos = WIDTH*PLANES_PER_PLAYER*NUMBER_OF_PLAYERS - 1;  // Wrap around horizontally
                yPos += ySpeed; if (yPos >= HEIGHT) animCounter=duration; // yPos = 0;
                playerArray[playerId].waveLower.addf((int)xPos, (int)yPos, impact);  // Set a wave peak at the current position
                playerArray[playerId].waveUpper.addf((int)xPos, (int)yPos, impact);  // Set a wave peak at the current position
            }
            #ifdef PLAY_IDLE_ANIM_NOTES
            if (animCounter == duration) {
                usbMIDI.sendNoteOff(idleAnimNote, MIDINOTE_VELOCITY, 8);  // Stop the MIDI note for idle animation
                idleAnimNote = 0;  // Reset the idle animation note
            }
            #endif
        }
    #endif
}


// Create a ripple effect at a random position within the central area of the display
void triggerWave(int xPos, int yPos, PlayerData * player) {
    
    /*
    // Define a margin percentage to keep ripples away from the edges
    float perc = .15f;

    // Calculate the boundaries for the ripple (15% from each edge)
    uint8_t min_x = perc * WIDTH * PLANES_PER_PLAYER;          // Left boundary
    uint8_t max_x = (1 - perc) * WIDTH * PLANES_PER_PLAYER;    // Right boundary
    uint8_t min_y = perc * PLAYER_MAX_YPOS;         // Top boundary
    uint8_t max_y = (1 - perc) * PLAYER_MAX_YPOS;   // Bottom boundary
    
    // Generate a random position within these boundaries
    int x = random(min_x, max_x);
    int y = pos==-1 ? random(min_y, max_y) : pos;
    */

    int xOffset = player->playerId * WIDTH * PLANES_PER_PLAYER;  // Offset for the player ID to separate wave layers
    Serial.printf("Triggering ripple at (%d, %d) for player %d\n", xPos, yPos, player->playerId);

    // Set a wave peak at this position in both wave layers (1.0 represents the maximum height of the wave)
    player->waveLower.setf(xPos + xOffset, yPos, TRIGGER_IMPACT_VALUE);  // Create ripple in lower layer
    player->waveUpper.setf(xPos + xOffset, yPos, TRIGGER_IMPACT_VALUE);  // Create ripple in upper layer
}

// Create a fancy cross-shaped effect that expands from the center
void applyBigWave(uint32_t now, PlayerData * player) {

    int xOffset = player->playerId * WIDTH * PLANES_PER_PLAYER;  // Offset for the player ID to separate wave layers

    // Find the center of the display
    int mid_x = WIDTH * PLANES_PER_PLAYER / 2;
    int mid_y = BIGWAVE_YPOS;
    
    // Calculate the maximum distance from center (half the width)
    int amount = WIDTH * PLANES_PER_PLAYER / 4;   // /2

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

void processPlayers(uint32_t now,PlayerData * player) {
    static int verticalPosition=0, horizontalPosition=0;
    static int joystickYValue=0, joystickXValue=0;
    int trigger1State=HIGH, trigger2State=HIGH;
    int TONESCALE_BASE=60;

    trigger1State = digitalRead(player->trigger1Pin);
    if (now - trigger1FlagsUpdateTime < EXTERNAL_TRIGGER_ACTIVE_PERIOD) 
        trigger1State= trigger1Flags & (1 << (player->playerId)) ? LOW : HIGH;  // Read trigger1 state from external flags


    if (player->joystickMode) {
        joystickYValue = analogRead(player->analogPin);
        joystickXValue = analogRead(player->trigger2Pin);  // use trigger2 pin for horizontal joystick movement
        verticalPosition   = (int)map (joystickYValue, 0, 1023, 2, PLAYER_MAX_YPOS);
        horizontalPosition = (int)map (joystickXValue, 1023, 0, 2, WIDTH * PLANES_PER_PLAYER);

        if (ENABLE_PITCHBEND) {
            int pitchbend = (int) map (joystickXValue, 1023, 0, -8192, 8191);
            static int oldPitchBend=0;
            if (oldPitchBend != pitchbend) {
                usbMIDI.sendPitchBend(pitchbend, player->midiChannel);       // MIDI channel = player id + 1
                oldPitchBend = pitchbend;
            }
        }

        if (trigger1State == LOW) {
            int xOffset = player->playerId * WIDTH * PLANES_PER_PLAYER;
            player->waveLower.addf( horizontalPosition + xOffset, verticalPosition, JOYSTICK_MOVEMENT_IMPACT);
            player->waveUpper.addf( horizontalPosition + xOffset, verticalPosition, JOYSTICK_MOVEMENT_IMPACT);

            if (ENABLE_TONE_WANDERING) {
                int actNote = player->tonescaleSize > 0 
                ? TONESCALE_BASE + player->tonescale[map (joystickYValue, 0, 1023, 0, player->tonescaleSize - 1)]  // Map to a note in the scale
                : map (verticalPosition, 0, 1023, 10, 90);  // take all midi tones in range 10-90 if no scale is defined
                if (actNote != player->trigger1Note) {
                    if (player->trigger1Note) usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel); 
                    player->trigger1Note = actNote;
                    usbMIDI.sendNoteOn(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel);  
                }
            }
        }
    } 
    else { 
        joystickYValue = random (0,1023);  // if no joystick attached, use random values!
        joystickXValue = random (0,1023);
        horizontalPosition = (int)map (joystickXValue, 1023, 0, 2, WIDTH * PLANES_PER_PLAYER);
        verticalPosition   = (int)map (joystickYValue, 0, 1023, 5, PLAYER_MAX_YPOS);
        trigger2State = digitalRead(player->trigger2Pin);
        if (now - trigger2FlagsUpdateTime < EXTERNAL_TRIGGER_ACTIVE_PERIOD) 
            trigger2State= trigger2Flags & (1 << (player->playerId)) ? LOW : HIGH;  // Read trigger2 state from external flags
    }
    
    if ((trigger1State == LOW) && (player->trigger1Active == 0)) {
        lastUserActivity = now;  // Update the last user activity timestamp
        player->trigger1Timestamp = now;  // remember the timestamp for trigger1
        player->trigger1Active = 1;

        if (!player->joystickMode) {
            verticalPosition  /= 2;
        }

        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_TRIGGER);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_TRIGGER);
        triggerWave(horizontalPosition, verticalPosition, player);  // create a wave at the determined position

        // Map the analog input to a MIDI note in the defined scale
        player->trigger1Note = player->tonescaleSize > 0 
            ? TONESCALE_BASE + player->tonescale[map (joystickYValue, 0, 1023, 0, player->tonescaleSize - 1)]  // Map to a note in the scale
            : map (verticalPosition, 0, 1023, 10, 90);  // take all midi tones in range 10-90 if no scale is defined

        if (!player->joystickMode) {
            player->trigger1Note -=12 ;  // lower the note by one octave for trigger1 in random mode
        }

        Serial.printf("Triggering player %d bottom half wave at analog value %d, mapped to note %d\n", player->playerId, verticalPosition, player->trigger1Note);
        usbMIDI.sendNoteOn(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel);    
    }   
    else if ((trigger1State == HIGH)  && (player->trigger1Active == 1)) {
        player->trigger1Active = 0;
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_RELEASE);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_RELEASE);
        usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel); 
    } 

    if ((trigger2State == LOW) && (player->trigger2Active == 0)) {
        lastUserActivity = now;  // Update the last user activity timestamp
        player->trigger2Timestamp = now;  // remember the timestamp for trigger2
        player->trigger2Active = 1;
        
        verticalPosition  = PLAYER_MAX_YPOS/2 + verticalPosition/2;

        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_TRIGGER);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_TRIGGER);
        triggerWave(horizontalPosition, verticalPosition, player);  // create a wave at the determined position

        // Map the analog input to a MIDI note in the defined scale
        player->trigger2Note = player->tonescaleSize > 0 
            ? TONESCALE_BASE + player->tonescale[map (joystickYValue, 0, 1023, 0, player->tonescaleSize - 1)]  // Map to a note in the scale
            : map (verticalPosition, 0, 1023, 10, 90);  // take all midi tones in range 10-90 if no scale is defined

        Serial.printf("Triggering Player %d top half wave at analog value %d, mapped to note %d\n", player->playerId, verticalPosition, player->trigger2Note);
        usbMIDI.sendNoteOn(player->trigger2Note, MIDINOTE_VELOCITY, player->midiChannel);  // MIDI channel = player id + 1
    }
    else if ((trigger2State == HIGH) && (player->trigger2Active == 1)) {
        player->trigger2Active = 0;  // Reset fancy button state
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_RELEASE);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_RELEASE);
        usbMIDI.sendNoteOff(player->trigger2Note, MIDINOTE_VELOCITY, player->midiChannel); 
    }

    #ifndef USE_BIG_MATRIX
        if (digitalRead(SWITCH_CHANNEL_BUTTON_PIN + player->playerId) == LOW) {
            static uint32_t lastButtonPressTime = 0;
            if (now - lastButtonPressTime > 1500) {  // Debounce delay
                usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel); 
                usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel); 
                player->midiChannel++; if (player->midiChannel > 5) player->midiChannel = 1; 
                lastButtonPressTime = now;
            }
        }
    #endif
}

// Setup function to initialize the wave effects and LED strip
void wavefx_setup() {

    Serial.print("Initial Free Ram = "); Serial.println(freeram());
    pinMode (USER_POTI_GND_PIN, OUTPUT);
    digitalWrite(USER_POTI_GND_PIN, LOW);  // Set the ground pin for the potentiometer

    #ifndef USE_BIG_MATRIX
        pinMode (SWITCH_CHANNEL_BUTTON_PIN, INPUT_PULLUP);    // Set button pin as input with pull-up resistor
        pinMode (SWITCH_CHANNEL_BUTTON_PIN+1, INPUT_PULLUP); // Set big button pin as input with pull-up resistor
        pinMode (SWITCH_CHANNEL_BUTTON_PIN+2, INPUT_PULLUP);    // Set button pin as input with pull-up resistor
    #endif

        // Initialize the LED strip (setScreenMap connects our 2D coordinate system to the 1D LED array)
    // TBD: verify correct format for Teensy4.1 5 parallel stripes
    // see: https://github.com/FastLED/FastLED/blob/master/examples/TeensyMassiveParallel/TeensyMassiveParallel.ino

    //auto screenmap = xyMap.toScreenMap();
    //CLEDController& c1 = FastLED.addLeds<WS2812, 8, GRB>(leds, NUM_LEDS_PER_PLANE).setScreenMap(screenmap);
    FastLED.addLeds<WS2812,  8, LEDSTRIPE_COLOR_LAYOUT>( leds, NUM_LEDS_PER_PLANE); //.setScreenMap(screenmap);
    FastLED.addLeds<WS2812,  9, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*1], NUM_LEDS_PER_PLANE); //.setScreenMap(screenmap);
    FastLED.addLeds<WS2812, 10, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*2], NUM_LEDS_PER_PLANE); //setScreenMap(screenmap);
    FastLED.addLeds<WS2812, 11, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*3], NUM_LEDS_PER_PLANE); //setScreenMap(screenmap);
    FastLED.addLeds<WS2812, 12, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*4], NUM_LEDS_PER_PLANE); //setScreenMap(screenmap);
    #ifndef USE_BIG_MATRIX
        FastLED.addLeds<WS2812, 7,  LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*5], NUM_LEDS_PER_PLANE); //setScreenMap(screenmap);
    #endif

    // Initialize the color palettes for the wave layers
    WaveCrgbMapPtr palYellowRed, palYellowWhite, palPurpleWhite, palBlueWhite, palDarkGreen, palDarkBlue, palDarkOrange, palDarkRed, palDarkPurple;  // Color palettes for the wave layers
    palYellowRed = fl::make_shared<WaveCrgbGradientMap>(yellowRedGradientPal);
    palYellowWhite = fl::make_shared<WaveCrgbGradientMap>(yellowWhiteGradientPal);
    palPurpleWhite = fl::make_shared<WaveCrgbGradientMap>(purpleWhiteGradientPal);
    palBlueWhite = fl::make_shared<WaveCrgbGradientMap>(blueWhiteGradientPal);
    palDarkBlue = fl::make_shared<WaveCrgbGradientMap>(darkBlueGradientPal);
    palDarkGreen = fl::make_shared<WaveCrgbGradientMap>(darkGreenGradientPal);
    palDarkRed = fl::make_shared<WaveCrgbGradientMap>(darkRedGradientPal);
    palDarkOrange = fl::make_shared<WaveCrgbGradientMap>(darkOrangeGradientPal);
    palDarkPurple = fl::make_shared<WaveCrgbGradientMap>(darkPurpleGradientPal);

    // Create parameter structures for each wave layer's blur settings
    Blend2dParams lower_params = {
        .blur_amount = BLUR_AMOUNT_LOWER,            // Blur amount for lower layer
        .blur_passes = BLUR_PASSES_LOWER,            // Blur passes for lower layer
    };

    Blend2dParams upper_params = {
        .blur_amount = BLUR_AMOUNT_UPPER,           // Blur amount for upper layer
        .blur_passes = BLUR_PASSES_UPPER,            // Blur passes for upper layer
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
        setWaveParameters(p.waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_RELEASE);  // default wave parameters for lower layer
        setWaveParameters(p.waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_RELEASE);  // Set wave parameters for upper layer

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

    setWaveParameters(bigWaveLower, BIGWAVE_SPEED_LOWER, BIGWAVE_DAMPING_LOWER);  // slower wave decay for big waves
    setWaveParameters(bigWaveUpper, BIGWAVE_SPEED_UPPER, BIGWAVE_DAMPING_UPPER);

    fxBlend.add(bigWaveLower);  // Add the big wave layers to the blender
    fxBlend.add(bigWaveUpper);
    fxBlend.setParams(bigWaveLower, lower_params);
    fxBlend.setParams(bigWaveUpper, upper_params);

    // Now set player-specific color palettes and tone scales
    playerArray[1].waveLower.setCrgbMap(palDarkGreen);
    playerArray[1].waveUpper.setCrgbMap(palYellowWhite);
    playerArray[2].waveLower.setCrgbMap(palDarkRed);
    playerArray[2].waveUpper.setCrgbMap(palPurpleWhite);

    #ifdef USE_BIG_MATRIX 
    playerArray[3].waveLower.setCrgbMap(palDarkOrange);
    playerArray[3].waveUpper.setCrgbMap(palYellowWhite);
    playerArray[4].waveLower.setCrgbMap(palDarkPurple);
    playerArray[4].waveUpper.setCrgbMap(palBlueWhite);
    #endif

    
    // Apply global blur settings to the blender
    fxBlend.setGlobalBlurAmount(0);       // Overall blur strength
    fxBlend.setGlobalBlurPasses(1);       // Number of blur passes

    FastLED.setBrightness(20);  // Default brightness for the LED strip
}

void wavefx_loop() {
    uint32_t now = millis();
    while (Serial1.available()) {
        int flags = Serial1.read();  // Read incoming bytes from Serial1
        if (flags & 0x80) { 
            trigger2Flags = flags;  // If the high bit is set, it's for trigger2
            trigger2FlagsUpdateTime = now;  // Update the timestamp for trigger2 flags
            Serial.printf("Received trigger2 flags: %02X\n", trigger2Flags);
        }
        else {
            trigger1Flags = flags;  // Otherwise, it's for trigger1
            trigger1FlagsUpdateTime = now;  // Update the timestamp for trigger1 flags
            Serial.printf("Received trigger1 flags: %02X\n", trigger1Flags);
        }
    }

    // Apply current settings and get button states for all players
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
        processPlayers(now, &playerArray[i]);
    }   


    if (ENABLE_BIGWAVE) {
        // Check if timestamps are close enough for bigWaves
        for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
            PlayerData * player1 = &playerArray[i];
            for (int j = i+1; j < NUMBER_OF_PLAYERS; j++) {
                PlayerData * player2 = &playerArray[j];

                // if timestamps are close
                if ((player1->trigger1Timestamp!=0 && player2->trigger1Timestamp!=0 && abs((long)(player1->trigger1Timestamp - player2->trigger1Timestamp)) < BIGWAVE_TIME_THRESHOLD) ||
                    (player1->trigger2Timestamp!=0 && player2->trigger2Timestamp!=0 && abs((long)(player1->trigger2Timestamp - player2->trigger2Timestamp)) < BIGWAVE_TIME_THRESHOLD)) {

                    // Start big wave animation 
                    player1->bigWaveTransition.trigger(now, 70, 0, 0);
                    player2->bigWaveTransition.trigger(now, 70, 0, 0);

                    // reset the trigger timestamps to avoid re-triggering
                    player1->trigger1Timestamp = player2->trigger1Timestamp = player1->trigger2Timestamp = player2->trigger2Timestamp = 0;

                    // Trigger the big wave MIDI notes
                    usbMIDI.sendNoteOff(bigwaveNote, MIDINOTE_VELOCITY, 7);  // in case note is still on, turn it off
                    bigwaveNote= 60 + player1->tonescale [bigWaveNoteIndex++ % 7];
                    usbMIDI.sendNoteOn(bigwaveNote, MIDINOTE_VELOCITY, 7);  // Send MIDI note for big wave effect, channel 7
                    bigWaveRunTime = now;  // Remember the time when the big wave was triggered
                }
            }
        }

        for (int i=0; i < NUMBER_OF_PLAYERS; i++) {
            if (playerArray[i].bigWaveTransition.isActive(now)) {  // Update the big wave transition state
                Serial.printf("big wave for player %d active!\n", i);
                applyBigWave(now, &playerArray[i]);
            }
        }

        if (bigWaveRunTime > 0 && now - bigWaveRunTime > BIGWAVE_MIDINOTE_DURATION) {
            bigWaveRunTime = 0;  // Reset the run time
            usbMIDI.sendNoteOff(bigwaveNote, MIDINOTE_VELOCITY, 7);  // Send MIDI note off for big wave effect, channel 7
        }
    }

    if (now - lastUserActivity > USER_ACTIVITY_TIMEOUT) {
        playIdleAnimation();  // Play idle animation if no user activity for a while
    } else {
        if (idleAnimNote != 0) {  // If idle animation note is set, turn it off!
            usbMIDI.sendNoteOff(idleAnimNote, MIDINOTE_VELOCITY, 8);  // Stop the MIDI note for idle animation
            idleAnimNote = 0;  // Reset the idle animation note
        }
    }

    #ifndef USE_RED_GREEN_IDLE_ANIMATION
        Fx::DrawContext ctx(now, leds); // Create a drawing context with the current time and LED array
        fxBlend.draw(ctx);      // Draw the blended result of both wave layers to the LED array
    #endif

    FastLED.show();         // Send the color data to the actual LEDs

    static int frameCount = 0;  // Frame counter for performance monitoring
    static int frameTime = 0;   // Time taken for the last frame
    frameCount++;

    // If debug output is enabled, print the frame rate and free RAM every second
    if (millis() - frameTime >= 1000) {
        int act_brightness = map (analogRead(USER_POTI_SIGNAL_PIN), 0, 1023, 0, MAXIMUM_BRIGHTNESS);  // Read brightness from potentiometer
        FastLED.setBrightness(act_brightness);  // Set the overall brightness (0-255)
        #ifdef CREATE_DEBUG_OUTPUT
            // Every second, print the frame rate
            Serial.printf("FPS: %d, Free Ram = %d, PixelPin=%d\n", frameCount, freeram(), NEOPIXEL_PIN);
        #endif

        frameCount = 0;  // Reset frame counter
        frameTime = millis();  // Update last frame time
    }
}


/*
    // Example of how to send MIDI program change messages to change the instrument
    instrument = (instrument + 1) % 10; // Cycle through instruments 0-9
    usbMIDI.sendProgramChange(instrument, 1);
    usbMIDI.sendControlChange(0, instrument, 1);  // Bank Select MSB = 1
    usbMIDI.sendControlChange(32, 0, 1); // Bank Select LSB = 0
*/
