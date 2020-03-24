#include<Arduino.h>
#include<Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(1, 88);

void setup(){
  pixels.begin();
}

void loop(){
  pixels.setPixelColor(0, pixels.Color(random(63), random(63) , random(63)));
  pixels.show();
  delay(1000); 
}
