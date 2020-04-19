#include<max6675.h>
#include<Arduino.h>

using namespace std; 

MAX6675 thermocouple(12, 13, 11);
int topElem = 0;
int botElem = 1;
int rotMot = 2;

enum bean {
  elsalvador,
  nicaragua
}; 

double desiredTemp(int time, bean b){
  // Keep temperature high for first 12 minutes, then aim at 430
  switch(b){
    default:
      if(time < 720) return 700;  
      else if(time < 1200) return 440;
      else return 0; 
  }
}

void heat(bool set){
  digitalWrite(topElem, set);
  digitalWrite(botElem, set);
}

void setup(){
  Serial.begin(9600);
  Serial.println("max 6675 test");
  pinMode(topElem, OUTPUT); digitalWrite(topElem, LOW);
  pinMode(botElem, OUTPUT); digitalWrite(botElem, LOW);
  pinMode(rotMot, OUTPUT); digitalWrite(rotMot, LOW);
  delay(1000); 
}

void roast(bean b){
  char *c; switch(b) {
    case elsalvador:
      c = "El Salvador";
  }
  Serial.print("Starting roast for ");
  Serial.println(c);

  for(int t = 0; t < 1200; t++){
    double actualTemp = thermocouple.readFarenheit(); 
    heat(desiredTemp(t, b) > actualTemp); 
    Serial.print(actualTemp); 
    Serial.print(", ");
    Serial.println(desiredTemp(t,b)); 
    delay(1000); 
  }
}

void loop(){
  switch(Serial.read()){
    case 'e': 
      roast(elsalvador);
  }
}
