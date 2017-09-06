/*
* This code will transmit a simple shot and light the LED when hit by the sensor
V4
  Should sense trigger, light blink, and send a pulse on the IR led.  Speaker is experimental
*/
/* Original Credits:
* Giving Credit where Credit Is Due
*
* Portions of this code were derived from code posted in the Arduino forums by Paul Malmsten.
* You can find the original thread here: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1176098434
*
* The Audio portion of the code was derived from the Melody tutorial on the Arduino wiki
* You can find the original tutorial here: http://arduino.cc/en/Tutorial/Melody
*/

#include <avr/interrupt.h>

int senderPin  = 0;    // Infrared LED
int speakerPin = 1;    // Buzzer
int triggerPin = 2;    // Trigger
int blinkPin   = 3;    // Visible LED 
int sensorPin  = 4;    // IR Sensor

int trigger;             // This is used to hold the value of the trigger read;
boolean fired  = false;  // Boolean used to remember if the trigger has already been read.
int waitTime = 300;     // The amount of time to wait between pulses
int endBit     = 3000;   // This pulse sets the threshold for a transmission end bit

int startBit   = 2000;   // This pulse sets the threshold for a transmission start bit
int one        = 1000;   // This pulse sets the threshold for a transmission that represents a 1
int zero       = 400;    // This pulse sets the threshold for a transmission that represents a 0

int ret[2];              // Used to hold results from IR sensing.

int playerLine = 14;     // Any player ID >= this value is a referee, < this value is a player;
int myCode     = 1;      // This is your unique player code;
int myLevel    = 1;      // This is your starting level;
int maxShots   = 6;      // You can fire 6 safe shots;
int maxHits    = 6;      // After 6 hits you are dead;
int myShots   = maxShots;// You can fire 6 safe shots;
int myHits    = maxHits; // After 6 hits you are dead;

int maxLevel   = 9;      // You cannot be promoted past level 9;
int minLevel   = 0;      // You cannot be demoted past level 0

int refPromote = 0;      // The refCode for promotion;
int refDemote  = 1;      // The refCode for demotion;
int refReset   = 2;      // The refCode for ammo reset;
int refRevive  = 3;      // The refCode for revival;

int replySucc  = 14;     // the player code for Success;
int replyFail  = 15;     // the player code for Failed;

void setup() {
//Outputs
  pinMode(blinkPin, OUTPUT);
  digitalWrite(blinkPin, LOW);
  pinMode(speakerPin, OUTPUT);
  digitalWrite(speakerPin, LOW);
  pinMode(senderPin, OUTPUT);
  digitalWrite(senderPin, LOW);
  
//Inputs
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(triggerPin, INPUT_PULLUP);

//Ready
  for (int i = 0;i < 3;i++) {
    digitalWrite(blinkPin, HIGH);
    delay(200);
    playTone(100, 1);
    playTone(200, 1);
    playTone(300, 1);
    digitalWrite(blinkPin, LOW);
    delay(200);
  }
}
void loop() {
  senseFire();
  senseIR();
}

//Firing portion
void senseFire() {
  trigger = digitalRead(triggerPin);
  if (trigger == LOW && fired == false) {
    fired = true;
    fireShot(1,1);
  } else if (trigger == HIGH) {
    if (fired == true) {
      //Serial.println("Button Released");
    }
    // reset the fired variable
    fired = false;
  }
}

void fireShot(int player, int level) {
  int encoded[8];
  digitalWrite(blinkPin, HIGH);
  for (int i=0; i<4; i++) {
    encoded[i] = player>>i & B1;   //encode data as '1' or '0'
  }
  for (int i=4; i<8; i++) {
    encoded[i] = level>>i & B1;
  }
  // send startbit
  oscillationWrite(senderPin, startBit);
  // send separation bit
  digitalWrite(senderPin, HIGH);
  delayMicroseconds(waitTime);
  // send the whole string of data
  for (int i=7; i>=0; i--) {
    if (encoded[i] == 0) {
      oscillationWrite(senderPin, zero);
    } else {
      oscillationWrite(senderPin, one);
    }
    // send separation bit
    digitalWrite(senderPin, HIGH);
    delayMicroseconds(waitTime);
  }
  oscillationWrite(senderPin, endBit);
  playTone(100, 1);
  //delay(200);
  digitalWrite(blinkPin, LOW);
}

void oscillationWrite(int pin, int time) {
  for(int i = 0; i <= time/26; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(13);
    digitalWrite(pin, LOW);
    delayMicroseconds(13);
  }
}

//Sensing portion
void senseIR() 
{
  int who[4];
  int what[4];
  //Wait for 50 micro seconds, then return if nothing found
  //50 micros seems too short
  if(pulseIn(sensorPin, LOW, startBit*10) <= startBit)
  {
    return;
  }
  digitalWrite(blinkPin, HIGH);
  int readTimeout = 10000;
  who[0]   = pulseIn(sensorPin, LOW, readTimeout);
  who[1]   = pulseIn(sensorPin, LOW, readTimeout);
  who[2]   = pulseIn(sensorPin, LOW, readTimeout);
  who[3]   = pulseIn(sensorPin, LOW, readTimeout);
  what[0]  = pulseIn(sensorPin, LOW, readTimeout);
  what[1]  = pulseIn(sensorPin, LOW, readTimeout);
  what[2]  = pulseIn(sensorPin, LOW, readTimeout);
  what[3]  = pulseIn(sensorPin, LOW, readTimeout);


  //Serial.println("---WHO---");
  for(int i=0;i<=3;i++) {
    //Serial.println(who[i]);
    if(who[i] > one) {
      who[i] = 1;
    } else if (who[i] > zero) {
      who[i] = 0;
    } else {
      // Since the data is neither zero or one, we have an error
      //Serial.println("unknown player");
      ret[0] = -1;
      digitalWrite(blinkPin, LOW);
      return;
    }
  }
  ret[0]=convert(who);
  for(int i=0;i<=3;i++) {
    if(what[i] > one) {
      what[i] = 1;
    } else if (what[i] > zero) {
      what[i] = 0;
    } else {
      ret[0] = -1;
      digitalWrite(blinkPin, LOW);
      return;
    }
  }
  ret[1]=convert(what);
  playTone(300, 10);
  digitalWrite(blinkPin, LOW);
  playTone(400, 10);
  return;
}

void playTone(int tone, int duration) {
//  for (long i = 0; i < duration * 1000L; i += tone * 2) {
  for (long i = 0; i < duration * 100L; i++) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

int convert(int bits[]) {
  int result = 0;
  int seed   = 1;
  for(int i=3;i>=0;i--) {
    if(bits[i] == 1) {
      result += seed;
    }
    seed = seed * 2;
  }
  return result;
}

void blink(int pin)
{
  for (int i = 0;i < 1;i++) {
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(200);
  }
}


