#include "MspFlash.h"
#include "Ultrasonico.h"
#define flash SEGMENT_D

unsigned int machinestate = 2;
unsigned int setparkingdistancestate = 0;
unsigned int laststate = -1;
byte presetparkingdistance = 10;
unsigned long starttimeinstate;
unsigned int parkeddist = 0;
int greenon = false;

Ultrasonico ultrasonico(P2_7, P1_7); //(trigPin,echoPin)

#define WARN_LED P1_5
#define STOP_LED P2_0
#define SET_DISTANCE P2_5

void setup()  { 
  Serial.begin(9600);
  Serial.print("\n\rinitializing hardware\n\r");
  pinMode(WARN_LED, OUTPUT);
  digitalWrite(WARN_LED, greenon);
  pinMode(STOP_LED, OUTPUT);
  digitalWrite(STOP_LED, 0);
  pinMode(SET_DISTANCE, INPUT_PULLUP);
  attachInterrupt(SET_DISTANCE, buttonpushed, FALLING); // Interrupt is fired whenever button is pressed
  pinMode(P1_4, INPUT);
  Flash.read(flash, &presetparkingdistance, 1);
  ultrasonico.Begin(); //ints the sensor
} 

void loop()  { 
  switch (machinestate) {
    case 1:
      if (machinestate != laststate){
        Serial.print("empty spot state\n\r");
      }
      machinestate = emptyspot();
      laststate = 1;
      break;
    case 2:
      if (machinestate != laststate){
        Serial.print("car approaching state\n\r");
      }
      machinestate = carapproaching();
      laststate = 2;
      break;
    case 3:
      if (machinestate != laststate){
        Serial.print("stop alert state\n\r");
        starttimeinstate = millis();
      }
      machinestate = stopalert();
      laststate = 3;
      break;
    case 4:
      if (machinestate != laststate){
        parkeddist = getdistance();
        Serial.print("car parked\n\r");
      }
      machinestate = parked();
      laststate = 4;
      break;
  }
  
  if (setparkingdistancestate == 1){
      setparkingdistance();
  }
  
}


// state 1
int emptyspot(){
    digitalWrite(WARN_LED, 0);
    Serial.print("scanning empty spot ");
    int dist = getdistance();
    Serial.print(dist);
    Serial.print(" preset park distance: ");
    Serial.println(presetparkingdistance);
    // if the object is closer than 225 cm car is approaching
    if (dist < 225){
        return 2;
    }
    delay(1000);
    return 1;
}

// state 2
int carapproaching(){
    //Serial.print("scanning approaching car ");
    int dist = getdistance();
    Serial.println(dist);
    //Serial.print(" preset park distance: ");
    //Serial.println(presetparkingdistance);
    // if there is nothing within 225 cm, spot is empty
    if (dist > 225){
        digitalWrite(WARN_LED, 0);
        return 1;
    }
    // if the object is closer than the preset parking distance alert stop
    if (dist <= presetparkingdistance){
        digitalWrite(WARN_LED, 0);
        return 3;
    }
    int distancefromstop = dist - presetparkingdistance;
    delay((distancefromstop * 5) + 5);
    digitalWrite(WARN_LED, greenon);
    greenon = !greenon;
    return 2;
}

// state 3
int stopalert(){
    // if the start time is later than the current time, the timer has overflowed
    if (millis() < starttimeinstate){
        starttimeinstate = millis();
    }
    // after being in the stop state for while, assume we are parked
    // 120,000 ms = 2 minutes
    if (millis() - starttimeinstate > 120000){
        digitalWrite(STOP_LED, 0);
        return 4;
    }
    
    digitalWrite(STOP_LED, 1);
    delay(200);
    Serial.print("alerting approaching car to stop ");
    int dist = getdistance();
    Serial.print(dist);
    Serial.print(" preset park distance: ");
    Serial.println(presetparkingdistance);
    // if the car backs up more than 5 centimeters from the preset parking distance, switch to approaching mode
    if (dist > presetparkingdistance + 5){
        digitalWrite(STOP_LED, 0);
        return 2;
    }
    return 3;
}

int parked(){
    int dist = getdistance();
    int deltadist = dist - parkeddist;
    deltadist = abs(deltadist);
    // if the car moves more than 5 centimeters from the distrance when parked switch to approaching mode
    if (deltadist > 5){
        return 2;
    }
    delay(1000);
    Serial.print("still parked ");
    Serial.print(dist);
    Serial.print(" deltadist: ");
    Serial.println(deltadist);
    return 4;
}

// ISR
void buttonpushed(){
    setparkingdistancestate = 1;    
}

void setparkingdistance(){
    Serial.println("scanning for distance to car");
    int dist = getdistance();
    byte b = dist;
    if (b < 10){
        b=10;
    }
    Flash.erase(flash); 
    Flash.write(flash, &b ,1);
    presetparkingdistance = b;
    setparkingdistancestate = 0;
    digitalWrite(STOP_LED, 0);
    for(int x = 0; x < 5; x++){
      delay(50);
      digitalWrite(STOP_LED, 1);
      delay(50);
      digitalWrite(STOP_LED, 0);
    }

}

int getdistance(){
    delay(5);
    int val = analogRead(P1_4);
    val = val/4;
    //return val;
    int dist = ultrasonico.Distancia(1);
     if (dist > 255){
        dist = 255;
     }
     return dist;
}

