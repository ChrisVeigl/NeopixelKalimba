
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