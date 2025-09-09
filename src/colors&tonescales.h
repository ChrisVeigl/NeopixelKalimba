

#pragma once
#ifndef COLORS_TONE_SCALES_H
#define COLORS_TONE_SCALES_H

#include <FastLED.h>      // Main FastLED library for controlling LEDs
#include <wavefx.h>

// Tone scales, define the MIDI note offsets for different musical scales

#ifdef USE_BIG_MATRIX   
    const int tonescaleChromatic[]       = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};     // chromatic scale
    const int tonescaleMajor[]           = {0, 2, 4, 5, 7, 9, 11};  // Major scale extended to 12 notes
    const int tonescaleMinorNatural[]    = {0, 2, 3, 5, 7, 8, 10};  // Natural minor scale extended to 12 notes
    const int tonescaleMinorHarmonic[]   = {0, 2, 3, 5, 7, 8, 11};  // Harmonic minor scale extended to 12 notes
    const int tonescaleMinorMelodic[]    = {0, 2, 3, 5, 7, 9, 11};  // Melodic minor scale extended to 12 notes
    const int tonescalePentatonicMajor[] = {0, 2, 4, 7, 9, 12};  // Pentatonic major scale extended to 12 notes
    const int tonescalePentatonicMinor[] = {0, 3, 5, 7, 10, 12};  // Pentatonic minor scale extended to 12 notes
    const int tonescaleBlues[]           = {0, 3, 5, 6, 7, 10, 12};   // Blues scale extended to 12 notes
    const int tonescaleWholeTone[]       = {0, 2, 4, 6, 8, 10, 12}; // Whole tone scale (harmonic combinations)
#else
    const int tonescaleChromatic[]       = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};     // chromatic scale
    const int tonescaleMajor[]           = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19};  // Major scale extended to 12 notes
    const int tonescaleMinorNatural[]    = {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19};  // Natural minor scale extended to 12 notes
    const int tonescaleMinorHarmonic[]   = {0, 2, 3, 5, 7, 8, 11, 12, 14, 15, 18, 19};  // Harmonic minor scale extended to 12 notes
    const int tonescaleMinorMelodic[]    = {0, 2, 3, 5, 7, 9, 11, 12, 14, 15, 17, 19};  // Melodic minor scale extended to 12 notes
    const int tonescalePentatonicMajor[] = {0, 2, 4, 7, 9, 12, 14, 16, 19, 21, 24, 26};  // Pentatonic major scale extended to 12 notes
    const int tonescalePentatonicMinor[] = {0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24, 27};  // Pentatonic minor scale extended to 12 notes
    const int tonescaleBlues[]           = {0, 3, 5, 6, 7, 10, 12, 15, 17, 18, 19, 22};   // Blues scale extended to 12 notes
    const int tonescaleWholeTone[]       = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22}; // Whole tone scale (harmonic combinations)
#endif  

// Color palettes define the gradient of colors used for the wave effects
// Each entry has the format: position (0-255), R, G, B

DEFINE_GRADIENT_PALETTE(darkBlueGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    32,  0,   0,   70,  // Dark blue (low wave height)
    128, 20,  57,  255, // Electric blue (medium wave height)
    255, 255, 255, 255  // White (maximum wave height)
};

DEFINE_GRADIENT_PALETTE(purpleWhiteGradientPal){
    0,   0,   0,   0,   // Black (lowest wave height)
    8,   128, 64,  64,  // Green with red tint (very low wave height)
    16,  255, 222, 222, // Pinkish red (low wave height)
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