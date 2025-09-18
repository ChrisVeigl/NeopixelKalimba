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

#include <Averager.h>  // Include the Averager class for smoothing signal traces

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
XYMap xyMap = XYMap::constructWithLookUpTable(WIDTH*NUMBER_OF_PLAYERS, HEIGHT, XYTable, 0);  // For the actual LED output (may be serpentine)
XYMap xyRect(WIDTH * NUMBER_OF_PLAYERS, HEIGHT, false);         // For the wave simulation (always rectangular grid)

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
bool bigWaveEnabled = 1;  // Flag to enable/disable big wave effect

uint32_t lastUserActivity=0;  // Timestamp of the last user interaction (button press, wave trigger, etc.)
int idleAnimNote = 0;  // MIDI note for idle animation, will be set later based on player tone scale

int tonescaleSelection = 0;  // Currently selected tonescale / mode
int teamToneProgress = 0; // Progress through the tone scale for team mode

Averager avgVolumePoti(25);  // Averagers for smoothing volume potentiometer readings
Averager avgModePoti(25);


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
    const int * tonescale = nullptr;  // Pointer to the current tone scale
    int tonescaleSize = 0;
    uint32_t trigger1Timestamp = 0;  // Timestamp for the last trigger1 event
    uint32_t trigger2Timestamp = 0;  // Timestamp for the last trigger2 event
    int toneProgress = 0;  // Progress through the tone scale for this player

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

PlayerData playerArray[NUMBER_OF_PLAYERS] = {
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 0, A9,  22, 21},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 1, A6,  19, 18},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 2, A3,  16, 15},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 3, A0,  41, 40},
    { xyMap, CreateDefWaveArgs(), CreateDefWaveArgs(), 4, A15, 38, 37 }
};

void setWaveParameters( WaveFx & waveLower, float speed, float dampening) {
    // Set the speed and dampening for one wave layer
    waveLower.setSpeed(speed);
    waveLower.setDampening(dampening);
}

void playIdleAnimation() {
    static uint32_t lastUpdateTime = 0;  // Timestamp of the last update

    #ifdef USE_RED_GREEN_IDLE_ANIMATION
    /*
        static int strobo=0;
        if (millis() - lastUpdateTime > 100) {  // Update every 10 ms
            lastUpdateTime = millis();  // Update the timestamp
            strobo=!strobo;
            if (strobo) {
                for (int xPos=0;xPos<WIDTH*NUMBER_OF_PLAYERS;xPos++)
                    for (int yPos=0;yPos<HEIGHT;yPos++)
                        leds[xyMap(xPos, yPos)] = CRGB(255, 255, 255);
            } else {
                for (int xPos=0;xPos<WIDTH*NUMBER_OF_PLAYERS;xPos++)
                    for (int yPos=0;yPos<HEIGHT;yPos++)
                        leds[xyMap(xPos, yPos)] = CRGB(0,0,0);
            }
        }
*/
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
                idleAnimNote = playerArray[playerId].tonescale [random(0,7)];
                usbMIDI.sendNoteOn(idleAnimNote, MIDINOTE_VELOCITY, 8);  // Send MIDI note for idle animation
                #endif
            }

            if (animCounter > 0 && animCounter < duration ) {
                xPos += xSpeed; if (xPos >= WIDTH*NUMBER_OF_PLAYERS) xPos = 0; if (xPos < 0) xPos = WIDTH*NUMBER_OF_PLAYERS - 1;  // Wrap around horizontally
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
    
    int xOffset = player->playerId * WIDTH;  // Offset for the player ID to separate wave layers
    Serial.printf("Triggering ripple at (%d, %d) for player %d\n", xPos, yPos, player->playerId);

    // Set a wave peak at this position in both wave layers (1.0 represents the maximum height of the wave)
    player->waveLower.setf(xPos + xOffset, yPos, TRIGGER_IMPACT_VALUE);  // Create ripple in lower layer
    player->waveUpper.setf(xPos + xOffset, yPos, TRIGGER_IMPACT_VALUE);  // Create ripple in upper layer
}

// Create a fancy cross-shaped effect that expands from the center
void applyBigWave(uint32_t now, PlayerData * player) {

    int xOffset = player->playerId * WIDTH;  // Offset for the player ID to separate wave layers

    // Find the center of the display
    int mid_x = WIDTH / 2;
    int mid_y = BIGWAVE_YPOS;
    
    // Calculate the maximum distance from center (half the width)
    int amount = WIDTH  / 4;

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
    static int triggerYValue=0;
    int trigger1State=HIGH, trigger2State=HIGH;

    int xOffset = player->playerId * WIDTH;

    // handle trigger1 

    trigger1State = digitalRead(player->trigger1Pin);
    if (now - trigger1FlagsUpdateTime < EXTERNAL_TRIGGER_ACTIVE_PERIOD) 
        trigger1State= trigger1Flags & (1 << (player->playerId)) ? LOW : HIGH;  // Read trigger1 state from external flags
    
    if ((trigger1State == LOW) && (player->trigger1Active == 0)) {
        lastUserActivity = now;  // Update the last user activity timestamp
        player->trigger1Timestamp = now;  // remember the timestamp for trigger1
        player->trigger1Active = 1;
        horizontalPosition = WIDTH/2;

        switch (tonescales[tonescaleSelection].mode)
        {
            case MODE_RANDOM:
                triggerYValue = random (0,1023);  //  use random values!
                verticalPosition   = (int)map (triggerYValue, 0, 1023, 5, PLAYER_MAX_YPOS/2);  // use bottom half of the display for trigger1
                player->trigger1Note = player->tonescale[map (triggerYValue, 0, 1023, 0, player->tonescaleSize - 1)];  // Map to a note in the scale
                player->trigger1Note -=12 ;  // lower the note by one octave for trigger1            
                break;

            case MODE_TEAM:
                player->trigger1Note = player->tonescale[teamToneProgress % player->tonescaleSize];  // Map to a note in the scale
                teamToneProgress++;
                if (teamToneProgress >= player->tonescaleSize) teamToneProgress = 0;
                verticalPosition = map (player->trigger1Note, getMinTone(player->tonescale), getMaxTone(player->tonescale), 5, PLAYER_MAX_YPOS);
                break;

            case MODE_CANON:
                player->trigger1Note = player->tonescale[player->toneProgress % player->tonescaleSize];  // Map to a note in the scale
                player->toneProgress++;
                if (player->toneProgress >= player->tonescaleSize) player->toneProgress = 0;
                verticalPosition = map (player->trigger1Note, getMinTone(player->tonescale), getMaxTone(player->tonescale), 5, PLAYER_MAX_YPOS);
                break;
        }

        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_TRIGGER);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_TRIGGER);
        triggerWave(horizontalPosition, verticalPosition, player);  // create a wave at the determined position

        for (int i=0;i<WIDTH;i++) { 
            player->waveLower.setf(xOffset+i, PLAYER_MAX_YPOS+10, 1.0);  // Create ripple in lower layer
            player->waveUpper.setf(xOffset+i, PLAYER_MAX_YPOS+10, 1.0);  // Create ripple in upper layer
        }
        for (int i=0;i<NUMBER_OF_PLAYERS;i++) { 
            bigWaveLower.addf(i*WIDTH+WIDTH/2, HEIGHT-10, 0.05);
            bigWaveUpper.addf(i*WIDTH+WIDTH/2, HEIGHT-10, 0.05);
        }

        Serial.printf("Player %d trigger1 wave at position %d, mapped to note %d\n", player->playerId, verticalPosition, player->trigger1Note);
        usbMIDI.sendNoteOn(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel);    
    }   
    else if ((trigger1State == HIGH)  && (player->trigger1Active == 1)) {
        player->trigger1Active = 0;
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_RELEASE);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_RELEASE);
        usbMIDI.sendNoteOff(player->trigger1Note, MIDINOTE_VELOCITY, player->midiChannel);
    }


    // handle trigger2

    trigger2State = digitalRead(player->trigger2Pin);
    if (now - trigger2FlagsUpdateTime < EXTERNAL_TRIGGER_ACTIVE_PERIOD) 
        trigger2State= trigger2Flags & (1 << (player->playerId)) ? LOW : HIGH;  // Read trigger2 state from external flags

    if ((trigger2State == LOW) && (player->trigger2Active == 0)) {
        lastUserActivity = now;  // Update the last user activity timestamp
        player->trigger2Timestamp = now;  // remember the timestamp for trigger2
        player->trigger2Active = 1;
        horizontalPosition = WIDTH/2;

        switch (tonescales[tonescaleSelection].mode)
        {
            case MODE_RANDOM:
                triggerYValue = random (0,1023);  //  use random values!
                verticalPosition   = (int)map (triggerYValue, 0, 1023, PLAYER_MAX_YPOS/2, PLAYER_MAX_YPOS);  // use top half of the display for trigger2
                player->trigger2Note = player->tonescale[map (triggerYValue, 0, 1023, 0, player->tonescaleSize - 1)];  // Map to a note in the scale
                break;

            case MODE_TEAM:
                player->trigger2Note = player->tonescale[teamToneProgress % player->tonescaleSize];  // Map to a note in the scale
                teamToneProgress++;
                if (teamToneProgress >= player->tonescaleSize) teamToneProgress = 0;
                verticalPosition = map (player->trigger2Note, getMinTone(player->tonescale), getMaxTone(player->tonescale), 5, PLAYER_MAX_YPOS);
                break;

            case MODE_CANON:
                player->trigger2Note = player->tonescale[player->toneProgress % player->tonescaleSize];  // Map to a note in the scale
                player->toneProgress++;
                if (player->toneProgress >= player->tonescaleSize) player->toneProgress = 0;
                verticalPosition = map (player->trigger2Note, getMinTone(player->tonescale), getMaxTone(player->tonescale), 5, PLAYER_MAX_YPOS);
                break;
        }

        // Set wave parameters for longer wave duration  
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_TRIGGER);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_TRIGGER);
        triggerWave(horizontalPosition, verticalPosition, player);  // create a wave at the determined position

        for (int i=0;i<WIDTH;i++) { 
            player->waveLower.setf(xOffset+i, PLAYER_MAX_YPOS+10, 1.0);  // Create ripple in lower layer
            player->waveUpper.setf(xOffset+i, PLAYER_MAX_YPOS+10, 1.0);  // Create ripple in upper layer
        }
        for (int i=0;i<NUMBER_OF_PLAYERS;i++) { 
            bigWaveLower.addf(i*WIDTH+WIDTH/2, HEIGHT-10, 0.05);
            bigWaveUpper.addf(i*WIDTH+WIDTH/2, HEIGHT-10, 0.05);
        }
            
        Serial.printf("Player %d trigger2 wave at position %d, mapped to note %d\n", player->playerId, verticalPosition, player->trigger2Note);
        usbMIDI.sendNoteOn(player->trigger2Note, MIDINOTE_VELOCITY, player->midiChannel);  // MIDI channel = player id + 1
    }
    else if ((trigger2State == HIGH) && (player->trigger2Active == 1)) {
        player->trigger2Active = 0;  // Reset fancy button state
        // Set wave parameters for faster wave decay
        setWaveParameters(player->waveLower, WAVE_SPEED_LOWER, WAVE_DAMPING_LOWER_RELEASE);
        setWaveParameters(player->waveUpper, WAVE_SPEED_UPPER, WAVE_DAMPING_UPPER_RELEASE);
        usbMIDI.sendNoteOff(player->trigger2Note, MIDINOTE_VELOCITY, player->midiChannel);
    }

}

void processBigWaves(unsigned long now) {
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
                usbMIDI.sendNoteOff(bigwaveNote, MIDINOTE_VELOCITY, BIGWAVE_MIDI_CHANNEL);  // in case note is still on, turn it off
                bigwaveNote= player1->tonescale [bigWaveNoteIndex++ % 7];
                usbMIDI.sendNoteOn(bigwaveNote, MIDINOTE_VELOCITY, BIGWAVE_MIDI_CHANNEL);  // Send MIDI note for big wave effect
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
        usbMIDI.sendNoteOff(bigwaveNote, MIDINOTE_VELOCITY, BIGWAVE_MIDI_CHANNEL);  // Send MIDI note off for big wave effect
    }
}


void updatePotentiometers(unsigned long now) {
    static int potiUpdateTime = 0;   // Time taken for the last potentiometer update
    static int volume = 100;   // Current volume level (0-127)
    static int oldVolumeValue = -1;      // Previous volume potentiometer value for change detection

    if (millis() - potiUpdateTime >= POTENTIOMETER_UPDATE_PERIOD) {
        potiUpdateTime = millis();  // Update last poti read time

        int volumeValue = avgVolumePoti.process(analogRead(VOLUME_POTI_PIN));

        static int counter=0;
        counter += POTENTIOMETER_UPDATE_PERIOD;
        if (counter >= POTENTIOMENTER_UI_UPDATE_PERIOD) {  
            counter=0;

            // volume control (is currently disabled)
            if (abs(volumeValue - oldVolumeValue) > 10) {
                oldVolumeValue = volumeValue;
                int act_volume = map (volumeValue, 0, 1023, 120, 0);  // Read volume from potentiometer
                if (act_volume != volume) {
                    volume = act_volume;
                    Serial.printf(" Changing volume to %d\n", act_volume);
                    usbMIDI.sendControlChange(7, volume, 16);
                }
            }
        }
    }
}

void updateMode (unsigned long now) {
    static uint32_t lastModechangeTimestamp=0;
    // mode / tonescale control
    if ((digitalRead(MODE_PIN) == LOW) && (now - lastModechangeTimestamp > 2000)) {
        lastModechangeTimestamp=now;
        tonescaleSelection = ( tonescaleSelection + 1 ) % numTonescales;
        Serial.printf(" Changing tonescale to %s\n", tonescales[tonescaleSelection].name);

        for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
            PlayerData * player = &playerArray[i];
            player->tonescale = tonescales[tonescaleSelection].tonescale;
            player->tonescaleSize = getTonescaleSize(player->tonescale);
            player->toneProgress = 0;
        }
        teamToneProgress = 0;
        usbMIDI.sendControlChange(11, tonescaleSelection, 16);

        // update big wave mode
        if (tonescales[tonescaleSelection].mode == MODE_RANDOM) {
            bigWaveEnabled = true;
        } else {
            bigWaveEnabled = false;
        }   
    }
}


void handleIdleAnimation(unsigned long now) {
    // process idle animation
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
}

void monitorPerformance() {
    static int frameCount = 0;  // Frame counter for performance monitoring
    static int frameTime = 0;   // Time taken for the last frame
    frameCount++;

    // If debug output is enabled, print the frame rate and free RAM every second
    if (millis() - frameTime >= 1000) {
        #ifdef CREATE_DEBUG_OUTPUT
            // Every second, print the frame rate
            Serial.printf("FPS: %d, Free Ram = %d, PixelPin=%d\n", frameCount, freeram(), NEOPIXEL_PIN);
        #endif

        frameCount = 0;  // Reset frame counter
        frameTime = millis();  // Update last frame time
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////

// Setup function to initialize the wave effects and LED strip
void wavefx_setup() {

    Serial.print("Initial Free Ram = "); Serial.println(freeram());
    pinMode (POTI_GND_PIN, OUTPUT);
    digitalWrite(POTI_GND_PIN, LOW);  // Set the ground pin for the potentiometer
    pinMode (MODE_PIN, INPUT_PULLUP);
    pinMode (MODE_GND_PIN, OUTPUT);
    digitalWrite(MODE_GND_PIN, LOW);  // Set the ground pin for the mode button


    // Initialize the LED strip (setScreenMap connects our 2D coordinate system to the 1D LED array)
    // see: https://github.com/FastLED/FastLED/blob/master/examples/TeensyMassiveParallel/TeensyMassiveParallel.ino

    FastLED.addLeds<WS2812,  8, LEDSTRIPE_COLOR_LAYOUT>( leds, NUM_LEDS_PER_PLANE);
    FastLED.addLeds<WS2812,  9, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*1], NUM_LEDS_PER_PLANE);
    FastLED.addLeds<WS2812, 10, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*2], NUM_LEDS_PER_PLANE);
    FastLED.addLeds<WS2812, 11, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*3], NUM_LEDS_PER_PLANE);
    FastLED.addLeds<WS2812, 12, LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*4], NUM_LEDS_PER_PLANE);
    FastLED.addLeds<WS2812, 7,  LEDSTRIPE_COLOR_LAYOUT>(&leds[NUM_LEDS_PER_PLANE*5], NUM_LEDS_PER_PLANE);

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
    playerArray[2].waveLower.setCrgbMap(palDarkOrange);
    playerArray[2].waveUpper.setCrgbMap(palYellowWhite);
    playerArray[3].waveLower.setCrgbMap(palDarkRed);
    playerArray[3].waveUpper.setCrgbMap(palPurpleWhite);
    playerArray[4].waveLower.setCrgbMap(palDarkPurple);
    playerArray[4].waveUpper.setCrgbMap(palBlueWhite);
    
    // Apply global blur settings to the blender
    fxBlend.setGlobalBlurAmount(0);       // Overall blur strength
    fxBlend.setGlobalBlurPasses(1);       // Number of blur passes

    FastLED.setBrightness(MAXIMUM_BRIGHTNESS);  // Default brightness for the LED strip
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
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)   
        processPlayers(now, &playerArray[i]);   
    
    updatePotentiometers(now);   // update potentiometers for user settings
    updateMode(now);
    handleIdleAnimation(now);    // handle / update idle animation


    if (bigWaveEnabled) processBigWaves(now); // process big waves if enabled
    if ((now-lastUserActivity>CANON_INACTIVITY_TIMEOUT) && (tonescales[tonescaleSelection].mode) == MODE_CANON) {
        for (int i=0;i<NUMBER_OF_PLAYERS;i++) {
            playerArray[i].toneProgress=0;
        }
    }

    FastLED.show();              // send the color data to the actual LEDs
    monitorPerformance();
}


