/*
   Copyright 2019 Leon Kiefer

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#define LED_NUM 164
#define LED_PIN 2



#define CLP_LED_NUM 96
#include <CorsairLightingProtocol.h>
#include <FastLED.h>




CRGB ledsChannel1[LED_NUM];
CRGB ledsChannel2[CLP_LED_NUM];

CorsairLightingFirmware firmware = corsairLightingNodePROFirmware();
FastLEDController ledController(true);
CorsairLightingProtocolController cLP(&ledController, &firmware);
CorsairLightingProtocolHID cHID(&cLP);

void setup() {
	FastLED.addLeds<WS2812B, LED_PIN, GRB>(ledsChannel1, LED_NUM);

	ledController.addLEDs(0, ledsChannel1, CLP_LED_NUM);
	ledController.addLEDs(1, ledsChannel2, CLP_LED_NUM);
}

void loop() {
	cHID.update();
  
	if (ledController.updateLEDs()) {

    for (int i = 0; i < LED_NUM; i++) {

      
      ledsChannel1[i] = ledsChannel1[i % CLP_LED_NUM];
      
      
    }
		FastLED.show();
	}
}
