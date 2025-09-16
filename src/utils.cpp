
#include <Arduino.h>      // Core Arduino functionality
#include "utils.h" 

int freeram() {
  return (char *)&_heap_end - __brkval;
}

float randomFloat(float min, float max) {
    return min + (max - min) * (random(0, 10000) / 10000.0f);
}

int getTonescaleSize(const int* tonescale) {
    int size = 0;
    while (tonescale[size] != -1) {
        size++;
    }
    return size;
}

int getMaxTone(const int* tonescale) {
    int maxTone = 0;
    int i = 0;
    while (tonescale[i] != -1) {
        if (tonescale[i] > maxTone) {
            maxTone = tonescale[i];
        }
        i++;
    }
    return maxTone;
}

int getMinTone(const int* tonescale) {
    int minTone = 127; // MIDI notes range from 0 to 127
    int i = 0;
    while (tonescale[i] != -1) {
        if (tonescale[i] < minTone) {
            minTone = tonescale[i];
        }
        i++;
    }
    return minTone;
}