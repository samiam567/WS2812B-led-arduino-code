
#ifndef MUSIC_LED_H_DEFINED
#define MUSIC_LED_H_DEFINED

// 0: OFF
// 1: Frequencyd-> bars coming from either side, color change with thresh vrms
// 2: Frequency -> bar starting from center going out, color change with thresh vrms
// 3: color change with Vrms
// 4: rainbow
// 5: solid color

// i love you alec

#define MAX_MUSIC_MODE 3

void musicLEDSsetup();
void runMusicLeds();

void resetNormalizationData();

CRGB getColorShift(double position);

#endif
