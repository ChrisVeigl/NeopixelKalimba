

#pragma once
#ifndef COLORS_TONE_SCALES_H
#define COLORS_TONE_SCALES_H

#include <FastLED.h>      // Main FastLED library for controlling LEDs
#include <wavefx.h>

// Tone scales, define the MIDI note offsets for different musical scales
const int tonescaleChromatic[]       = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, -1};     // chromatic scale
const int tonescaleMajor[]           = {60, 62, 64, 65, 67, 69, 71, -1};  // Major scale extended to 12 notes
const int tonescaleMinorNatural[]    = {60, 62, 63, 65, 67, 68, 70, -1};  // Natural minor scale extended to 12 notes
const int tonescaleMinorHarmonic[]   = {60, 62, 63, 65, 67, 68, 71, -1};  // Harmonic minor scale extended to 12 notes
const int tonescaleMinorMelodic[]    = {60, 62, 63, 65, 67, 69, 71, -1};  // Melodic minor scale extended to 12 notes
const int tonescalePentatonicMajor[] = {60, 62, 64, 67, 69, 72, -1};      // Pentatonic major scale extended to 12 notes
const int tonescalePentatonicMinor[] = {60, 63, 65, 67, 70, 72, -1};      // Pentatonic minor scale extended to 12 notes
const int tonescaleBlues[]           = {60, 63, 65, 66, 67, 70, 72, -1};  // Blues scale extended to 12 notes
const int tonescaleWholeTone[]       = {60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, -1}; // Whole tone scale (harmonic combinations)

const int kalimbaNotes_team1[] = { 60, 72, 64, 76, 67, 79, 60, 72, 64, 76, 67, 79, 60, 72, 64, 76, 81, 69, 60, 72, 64, 76, 81, 69, 60, 65, 77, 69, 81, 72, 60, 65, 77, 69, 81, 72, 74, 62, 77, 65, 81, 69, 74, 62, 77, 65, 81, 69 , -1};
const int kalimbaNotes_team2[] = { 60, 72, 64, 69, 67, 65, 71, 62, 67, 65, 64, 69, 60, 72, 64, 69, 67, 65, 71, 62, 67, 65, 64, 69, 60, 76, 64, 72, 67, 69, 74, 65, 71, 69, 67, 72, 60, 76, 64, 72, 67, 69, 74, 65, 71, 69, 67, 72 , -1 };
const int kalimbaNotes_canon1[] = { 53, 55, 57, 53, 53, 55, 57, 53, 57, 58, 60, 57, 58, 60, 60, 62, 60, 58, 57, 53, 60, 62, 60, 58, 57, 53, 55, 48, 53, 55, 48, 53, -1 };

struct TonescaleStruct {
    const char* name;
    const int* tonescale;
    int mode;
};

const TonescaleStruct tonescales[] = {
    {"Pentatonic Major", tonescalePentatonicMajor, MODE_RANDOM},
    {"Whole Tone", tonescaleWholeTone, MODE_RANDOM},
    {"Kalimba Team 1", kalimbaNotes_team1, MODE_TEAM},
    {"Kalimba Team 2", kalimbaNotes_team2, MODE_TEAM},
    {"Kalimba Canon 1", kalimbaNotes_canon1, MODE_CANON}
};

int numTonescales = sizeof(tonescales) / sizeof(tonescales[0]);

// Color palettes define the gradient of colors used for the wave effects
// Each entry has the format: position (0-255), R, G, B

DEFINE_GRADIENT_PALETTE(darkBlueGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    32,  0,   0,   50,  // Dark blue (low wave height)
    128, 20,  57,  255, // Electric blue (medium wave height)
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(purpleWhiteGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    8,   60, 40,  30,  // Green with red tint (very low wave height)
    16,  180, 100, 180, // Pinkish red (low wave height)
    120,  255, 255, 255, // White (medium wave height)   //64
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(yellowRedGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    8,   20, 0,  0,  // Green with red tint (very low wave height)
    16,  140, 100, 0, // Pinkish red (low wave height)
    255, 255, 255, 255  // White (maximum wave height)
};


DEFINE_GRADIENT_PALETTE(darkGreenGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    32,  0,   70,  0,   // Dark green (low wave height)
    128, 20,  255,  40, // Electric green (medium wave height)
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(yellowWhiteGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    8,   80, 80,  20,  // yellow with red tint (very low wave height)
    16,  180, 150, 40, // Pinkish red (low wave height)
    120,  255, 255, 255, // White (medium wave height)   //64
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(darkRedGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    32,  70,   0,   0,  // Dark red (low wave height)
    128, 255,  57,  80, // Electric red (medium wave height)
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(darkPurpleGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    32,  70,   0,   70,  // Dark purple (low wave height)
    128, 255,  57,  255, // Electric purple (medium wave height)
    255, 255, 255, 255  // White (maximum wave height)
};


DEFINE_GRADIENT_PALETTE(blueWhiteGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    8,   128, 64,  64,  // Green with red tint (very low wave height)
    16,  220, 200, 255, // Bluish red (low wave height)
    120,  255, 255, 255, // White (medium wave height)     // 64
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(darkOrangeGradientPal){
    0,   0,   0,   0,    // Black (lowest wave height)
    32,  120,  60,   0,  // Dark orange (low wave height)
    128, 255,  120,  20, // Electric orange (medium wave height)
    255, 255, 255, 255   // White (maximum wave height)
};


#endif