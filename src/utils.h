
#ifndef UTILS_H
#define UTILS_H

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

int freeram();
float randomFloat(float min, float max) ;
int getTonescaleSize(const int* tonescale);

#endif
