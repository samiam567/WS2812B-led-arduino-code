
#ifndef MUSIC_LED_H_DEFINED
#define MUSIC_LED_H_DEFINED


void musicLEDSsetup();
void runMusicLeds();

void resetNormalizationData();

CRGB getColorShift(unsigned double position);


#endif
