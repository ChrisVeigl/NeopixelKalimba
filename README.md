# Neopixel Kalimba

The Neopixel Kalimba is an interactive light- and sound installation made for the Zoom Kindermuseum Vienna.
It uses the [FastLED library](https://github.com/FastLED/) to display visual effects on a large NeoPixel matrix,
and creates MIDI output for audio effects. The visuals are using layered WaveFx effects of the FastLED library 
(upper and lower layer) per player. The number of players is configurable, each player can use different colors palettes and wave parameters,
which are blended together to create complex visual effects.
Waves can be triggered by buttons or piezo sensors, and the wave parameters can be adjusted using analog inputs (e.g., an analog joystick).

## Requirements

This firmware uses the FastLED library for LED control and supports MIDI output for controlling MIDI devices (e.g. the ZynAddSubFX synthesizer).
It is designed to run on a Teensy 4.1 and supports Neopixel matrices up to 2000 pixels.

# Install Guide

  * Download and install VSCode
  * Open up VSCode and install the platformio extension
  * Download this repo or `git clone` it
  * Build using the PlatformIO tools
  * For more information see https://github.com/FastLED/PlatformIO-Starter
   
   