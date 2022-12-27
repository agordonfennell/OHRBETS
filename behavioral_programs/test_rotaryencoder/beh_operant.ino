/*



*/

// dependencies
  #include <Wire.h>
  #include "Adafruit_MPR121.h"
  #include <Servo.h>

// session parameters ************************************************************************************************************
// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinRotaryEncoderA = 3; 
  static byte pinRotaryEncoderB = 2; 
  
 // outputs ---------------------------
  static byte pinServo_retract = 9; 
  static byte pinServo_break = 10;
  static byte pinServo_radial = 11;
  
// servo break variables / parameters ------------------------------------------------------
  Servo servo_break;  // create servo object to control a servo
  static byte servo_break_disengaged_deg = 0;

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_retracted_deg = 120;

// servo radial varialbe / parameters-------------------------------------------------------
  Servo servo_radial;
  static byte servo_radial_deg = 0;
  
// rotary encoder variables / parameters ----------------------------------------------------
 // variables---
  volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderA to signal that the encoder has arrived at a detent
  volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
  volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
  
  uint8_t maskA;  // mask for reading pin A
  uint8_t maskB;  // mask for reading pin B
  uint8_t maskAB; // combined mask for pin A and B
  volatile uint8_t *port;
  
// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile boolean rotation_left_flag;
  volatile boolean rotation_right_flag;
  

  

// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);

 // define inputs --------------------------------
  pinMode(pinRotaryEncoderA,INPUT_PULLUP);
  pinMode(pinRotaryEncoderB,INPUT_PULLUP);
  
 // define outputs
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_break, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 

 // rotary encoder ------------------------------
 // setup interupts for the rotary encoder 
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderB),fpinRotaryEncoderB,RISING); // interrupt on pinRotaryEncoderA
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderA),fpinRotaryEncoderA,RISING); // interrupt on pinRotaryEncoderB
  
 // save starting state of rotary encoder
  maskA = digitalPinToBitMask(pinRotaryEncoderA);
  maskB = digitalPinToBitMask(pinRotaryEncoderB);
  maskAB = maskA | maskB;
  port = portInputRegister(digitalPinToPort(pinRotaryEncoderA));

// engage servo break prior to session start
  servo_break.attach(pinServo_break);    
  servo_break.write(servo_break_disengaged_deg);
  delay(250);
  servo_break.detach();

 // rotate multi-spout head prior to session start
  servo_radial.attach(pinServo_radial);
  servo_radial.write(servo_radial_deg);
  delay(250);
  servo_radial.detach();

 // fully extend spout prior to session start
  servo_retract.attach(pinServo_retract); 
  servo_retract.write(servo_retract_retracted_deg);
  delay(250);
  servo_retract.detach();

  delay(50);

  Serial.println("Test for rotary encoder (direction relative to mouse orientation)");
}


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
  if(rotation_left_flag){
    Serial.println("Rotation Left");
    rotation_left_flag = 0; // reset left flag to close if statement
   }
   
  if(rotation_right_flag){
    Serial.println("Rotation Right");
    rotation_right_flag = 0; // reset left flag to close if statement
   }
}


// _______________________________________________________________________________________________________________________________________________________________________________
/// functions & interupts_________________________________________________________________________________________________________________________________________________________
// rotary encoder interupts ----------------------------------------------
void fpinRotaryEncoderB(){ //---------------------------------------------
  noInterrupts();
  reading = *port & maskAB;
  if((reading == maskAB) && aFlag) {

  rotation_left_flag = 1; // flag leftward rotation
  
  bFlag = 0;
  aFlag = 0;
  }
  else if (reading == maskB) bFlag = 1;
  interrupts();
}

void fpinRotaryEncoderA(){ //---------------------------------------------
  noInterrupts();
  reading = *port & maskAB;
  if (reading == maskAB && bFlag) {
    
  rotation_right_flag = 1; // flag rightward rotation
  
  bFlag = 0;
  aFlag = 0;
  }
  else if (reading == maskA) aFlag = 1;
  interrupts();
}
