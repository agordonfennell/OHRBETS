/*
Description:
  program allows user to rotate multi-spout head and open individual solenoids for setup or cleanup using one of two modes

  currently setup for 5 solenoids. Works with less solenoids, adjust pinSol, sol_count, and stateSol to work with more

Instructions (see protocol_helper_opensol for step by step instructions):
- set open_mode and servo_radial_deg, and send to arduino
- open serial monitor
- mode 0:
  - touch spout to open corresponding solenoid, touch again to close
  - set multi-spout angle by sending a degree between 1 and 180 over serial
- mode 2:
  - set multi-spout angle using servo_radial_deg
  - open and close solenoids by sending a serial string with an ID between 1 and n, where n is the nuber of spouts set in pinSol and stateSol
- during operation, a string of binary values will be printed over serial 
  - the position of the value indicates the solenoid
  - the value indicates the state (1 = open, 0 = closed)
  - e.g. 00010 means solenoid 4 is open

*/
 // libraries
  #include <Wire.h>
  #include "Adafruit_MPR121.h"
  #include <Servo.h>

 // parameters ************************************************************************************************
  boolean open_mode = 0; // 0: touch to open set radial w/ serial, 1: serial to open (must use 1 for voltage sensor)
  byte servo_radial_deg = 120; // starting multi-spout angle (mode 0); fixed multi-spout angle (mode 1)
  int sol_count = 5; // number of sol (must match length of pinSol & stateSol)
 // ***********************************************************************************************************
 
 // Pins
  static byte pinSol[]   = {4,5,6,7,8}; // one pin per solenoid
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;

 // variables
  boolean stateSol[] = {0,0,0,0,0}; // one state per sol
  byte switch_sol_id;
  byte switch_sol_id_serial;
  byte lick_last;
  boolean serial_toggle = 0;
  unsigned long baud = 115200;

 // servos 
   // servo retractable spout variables / parameters ------------------------------------------
    Servo servo_retract;
    static byte servo_retract_retracted_deg = 120;
    
  
   // servo brake variables / parameters ------------------------------------------------------
    Servo servo_brake;  // create servo object to control a servo
    static byte servo_brake_engaged_deg = 13;
  
   // servo radial  ---------------------------------------------------------------------------
    Servo servo_radial;

  
 // capsensor
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;

void setup() { //--------------------------------------------------------------------------------------------------
  Serial.begin(baud); // start serial
  Serial.println("Serial Begin");

  
 // set inputs / outputs
  for(uint8_t i = 0; i<=sol_count-1; i++){
    pinMode(pinSol[i], OUTPUT);
  }
  
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT);  


  if(open_mode == 0){
   // check for cap sensor
    if (!cap.begin(0x5A)) {
      Serial.println("MPR121 not found, check wiring?");
      while (1);
    }
    
    Serial.println("MPR121 found!");
  }

 // attach and move servos
   // retract
    servo_retract.attach(pinServo_retract);
    servo_retract.write(servo_retract_retracted_deg);
    delay(250);
    servo_retract.detach();
  
   // brake
    servo_brake.attach(pinServo_brake);
    servo_brake.write(servo_brake_engaged_deg);
    delay(250);
    servo_brake.detach();

   // radial
    servo_radial.attach(pinServo_radial);
    servo_radial.write(servo_radial_deg);
    delay(250);
    servo_radial.detach();

 // print prompt based on mode
  if(open_mode == 0){
    Serial.println("Send Serial to Update Radial Deg (0-180)");
  }
  
  if(open_mode == 1){
    Serial.println("Send Serial to Open Spouts (1-5)");
  }
}

void loop() { //--------------------------------------------------------------------------------------------------
  // open mode 1 (serial to open sol)***********************
   if(Serial.available() && open_mode == 1){
    switch_sol_id_serial = Serial.parseInt(); // read serial
    
      if(switch_sol_id_serial > 0 && switch_sol_id_serial <= sol_count){ // if input is within range of the number of sol
        switch_sol_id = switch_sol_id_serial; // flag sol number for state switch
      }
   }


 // open mode 0 (touch spout to open sol)***********************
  // set servo_radial_deg using serial input
   if(Serial.available() && open_mode == 0){
      int servo_radial_deg = Serial.parseInt();
      if(servo_radial_deg > 0){
        Serial.print("Current Deg: "); Serial.println(servo_radial_deg);
        servo_radial.attach(pinServo_radial);
        servo_radial.write(servo_radial_deg);
        delay(500);
        servo_radial.detach();
      }
   }
  // touch spout to open sol
    if(open_mode == 0){
      currtouched = cap.touched();
      for (uint8_t i = 1; i <= sol_count; i++) { // for each sensor (change the maximum i if more touch sensors are added)
        if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) { // if touched now but not previously
          switch_sol_id = i; // flag touch
        }
      }
      lasttouched = currtouched;
    }

 // general function to open sol *******************************
  if(switch_sol_id>0){ // flag from touch or serial
   // invert state for sol defined by flag
    stateSol[switch_sol_id-1] = !stateSol[switch_sol_id-1]; 
    digitalWrite(pinSol[switch_sol_id-1],stateSol[switch_sol_id-1]);

   // print state of sols
    Serial.print("Pin States: ");
    
    for(uint8_t i = 0; i<=sol_count-1; i++){
      Serial.print(stateSol[i]);
    }
    
    Serial.println();
    
    switch_sol_id = 0; // reset switch_sol_id flag to close if statement
  }
  
}
