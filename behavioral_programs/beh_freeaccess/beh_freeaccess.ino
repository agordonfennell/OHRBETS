/*
-General notes--------------------------
* This program is a spout training task, where each lick triggers a solenoid opdning
* brake is engaged, spout is extended, and multi-spout head is rotated at start of session
* spout is retracted at end of session
* setup to accomidate setups with multiple spouts
  ~ see section spout / sol pins & parameters, fill out 1 element per spout on system

-Dependencies---------------------------
This program uses multiple dependencies (see protocol: for instructions on installation)
* Servo.h: library used to control micro servos used for brake / retractable spout 
* Wire.h: library used for capacitive touch sensor
* Adafuit_MPR121.h: library used for capacitive touch sensor

-Data structure-------------------------
* see general arduino program approach for a description of how events and parameters are recorded

-Running the program--------------------------
* follow protocol for behavior script

*/

// dependencies
  #include <Wire.h>
  #include "Adafruit_MPR121.h"
  #include <Servo.h>

// session parameters *************************************************************************************************************
 // general parameters -------
  static unsigned long session_duration = 600000; // session duration (ms) (note: do not use formula here)
  static byte lick_detection_circuit = 0; // 0: cap sensor, 1: voltage sensor
  static int tm_lick_latency_min = 50;

 // spout / sol pins & parameters
  static byte num_spouts = 5; // should have this many values for each of the following vectors

  // spout vectors (each element will correspond to a spout, add or remove elements based on system, default is setup for 5 spouts)
  static byte pinSol[] =                      {  4,  5,  6,  7,  8};
  static byte sol_duration[] =                { 30, 30, 30, 30, 30}; // calibrate to ~1.5µL / delivery
  static byte servo_radial_degs [] =          {  0, 30, 60, 90,120};
  static byte servo_retract_extended_degs[] = {180,180,180,180,180};
  static byte pinLickometer_ttl[] =           { 22, 22, 22, 22, 22};
  
  byte current_spout = 1; // set spout for session (1 to n, where n is the number of spouts on the system; used to index each vector above)

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinLickometer = 21; // only set to output if lick_detection_circuit = 1
  
 // outputs ---------------------------
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;

  // ttls for external time stamps
  static byte pinSol_ttl = 23; // single output used for all solenoid onset

//capicitance sensor variables -------------------------------------------------------------
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;

// servo brake variables / parameters ------------------------------------------------------
  Servo servo_brake;  // create servo object to control a servo
  static byte servo_brake_engaged_deg = 15;

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_retracted_deg = 160;
  unsigned long ts_servo_retract_retracted;

// servo radial varialbe / parameters-------------------------------------------------------
  Servo servo_radial;
  
// flags -------------------------------------------------------------------------------------
 // variables---
  boolean lick;
  boolean lick_gate = 1;
  boolean pinLickometer_state;
  boolean pinLickometer_state_previous;
  
// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile unsigned long ts; 
  unsigned long ts_start;
  unsigned long ts_sol_offset;
  unsigned long ts_sol_ttl_off;
  unsigned long ts_lickomter_ttl_off; 
  unsigned long ts_lick_gate_open = 0;
  unsigned long session_end_ts;

 // parameters---
  static int ttl_duration = 5; // duration of tttl pulses for external time stamps (ms)


// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  
 // define inputs
  if(lick_detection_circuit == 1){
    pinMode(pinLickometer, INPUT);
  }
  
 // define outputs
  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 

  for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each solenoid
    pinMode(pinSol[i_sol], OUTPUT);                       // define sol as output
    pinMode(pinLickometer_ttl[i_sol], OUTPUT);           // 
  } 

 // engage servo brake prior to session start
  servo_brake.attach(pinServo_brake); 
  Serial.print(11); Serial.print(" "); Serial.println(ts);      
  servo_brake.write(servo_brake_engaged_deg);
  delay(250);
  servo_brake.detach();

 // rotate multi-spout head prior to session start
  servo_radial.attach(pinServo_radial);
  servo_radial.write(servo_radial_degs[current_spout-1]);
  Serial.print(130); Serial.print(" "); Serial.println(ts);            delay(3);   // print radial position moved
  Serial.print(127); Serial.print(" "); Serial.println(ts);            delay(3);
  Serial.print(127); Serial.print(" "); Serial.println(current_spout); delay(3);
  delay(250);
  servo_radial.detach();
 
 // fully extend spout prior to session start
  servo_retract.attach(pinServo_retract); 
  Serial.print(13); Serial.print(" "); Serial.println(ts);      
  servo_retract.write(servo_retract_extended_degs[current_spout]);
  delay(250);
  servo_retract.detach();

  // setup capative touch sesnsor 
  if(lick_detection_circuit == 0){
    if (!cap.begin(0x5A)) {                   // if the sensor is not detected
      Serial.println("MPR121 not detected!"); // print warning (and intentionally crash python program)
      while (1);
    }
    cap.setThresholds(6,2); // set thresholds for cap sensor, adjust these based on sensitivity
    delay(50);
    Serial.print(998);   Serial.print(" "); Serial.println(cap.filteredData(1)); // print cap sensor starting value
  } else {
    delay(50);
  }

 // wait for serial command before initating session---------------------------------------------------------------------
  while (Serial.available() <= 0) {} // wait for serial input to start session
  delay(100);
  
 // save start time and send ttl to initate scope
  ts_start = millis();  
}


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
// background functions (run constantly independent of task events)--------------------------------------------
 // generate timestamp ---------------------------
  ts = millis()-ts_start;

  // open lick gate -------------------------------
  if (ts > ts_lick_gate_open && ts_lick_gate_open != 0) {
    lick_gate = 1;
    ts_lick_gate_open = 0;
  }
  
 // close solenoids---------------------------
  if (ts >= ts_sol_offset && ts_sol_offset != 0) {           // if time is after solenoid offset time
    digitalWrite(pinSol[current_spout - 1], LOW);              // set state to low
    Serial.print(14); Serial.print(" "); Serial.println(ts); // print sol offset
    ts_sol_offset = 0;                                       // reset solenoid offset time to close if statement
  }

 // turn off ttls for external time stamps ------------------------
  // lick---
  if (ts >= ts_lickomter_ttl_off && ts_lickomter_ttl_off != 0) {
    digitalWrite(pinLickometer_ttl[current_spout-1], LOW); // write ttl low
    ts_lickomter_ttl_off = 0;            // reset off time to close if statement
  }
  
 // solenoid---
  if(ts>=ts_sol_ttl_off && ts_sol_ttl_off!=0){
    digitalWrite(pinSol_ttl,LOW);  // write ttl low
    ts_sol_ttl_off = 0;            // reset off time to close if statement
  }
  
// session initialization (runs once at start) -----------------------------------------------------------------
  if(session_end_ts == 0){
    Serial.print(1);   Serial.print(" "); Serial.println(ts);                          // print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);            // print session duration
    Serial.print(116); Serial.print(" "); Serial.println(sol_duration[current_spout-1]); // print sol_duration

    session_end_ts = ts + session_duration;
  }


// licking-----------------------------------------------------------------------------------------------------
 if(lick_detection_circuit == 0){ // if using cap sensor --------------------------------------------------------------
   // check state of sensor to see if it is currently touched
    currtouched = cap.touched(); 
  
    // check to see if touch onset occured
    if ((currtouched & _BV(current_spout)) && !(lasttouched & _BV(current_spout)) ) { // if touched now but not previously
      lick = current_spout;                                                           // flag lick
    }
  
   // save current state for comparision with next state
    lasttouched = currtouched; 
 }

 if(lick_detection_circuit == 1){ // if using voltage sensor ----------------------------------------------------------
  // check state of sensor to see if it is currently touched
   pinLickometer_state = digitalRead(pinLickometer);

  // check to see if touch onset occured
   if(pinLickometer_state > pinLickometer_state_previous) { // if touched now but not previously
        lick = current_spout+1;                             // flag lick
   }
   
  // save current state for next loop 
   pinLickometer_state_previous = pinLickometer_state;
 }

 // programed consequences to licking
  if (lick > 0) { // if lick has occured
    if(lick_gate){      
      digitalWrite(pinLickometer_ttl[current_spout - 1], HIGH);                          // turn on ttl for external ts
      Serial.print(30 + current_spout); Serial.print(" "); Serial.println(ts); // print lick onset
      ts_lickomter_ttl_off = ts + ttl_duration;                       // set ttl offset time
  
      digitalWrite(pinSol[current_spout - 1], HIGH);                                    // open solenoid for touched spout
      Serial.print(40 + current_spout); Serial.print(" "); Serial.println(ts); // print sol opening onset
      ts_sol_offset = ts + sol_duration[current_spout - 1];                    // set solenoid close time
    }

    lick_gate = 0; // close lick gate until tm_lick_latency_min ms have passed

    if (tm_lick_latency_min > sol_duration[lick - 1]) {
      ts_lick_gate_open = ts + tm_lick_latency_min;
    } else {
      ts_lick_gate_open = ts +  sol_duration[lick - 1];
    }

    lick = 0; // reset lick flag to close if statement
    
  }

 // session termination ---------------------------------------------------------------------------------------------------
  // end session if end time is reached
  if(ts > session_end_ts && session_end_ts != 0){
    fun_end_session();
    Serial.print(99); Serial.print(" "); Serial.println(ts);    // print end of session  
  }
}


// _______________________________________________________________________________________________________________________________________________________________________________
/// functions & interupts_________________________________________________________________________________________________________________________________________________________
/// end session -------------------------------------------------------------------------------------------
void fun_end_session() {
  for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each solenoid
    digitalWrite(pinSol[i_sol], LOW);                    // turn off solenoid
  } 
  
  servo_brake.attach(pinServo_brake);  
  servo_brake.write(servo_brake_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);      
  delay(250);
  servo_brake.detach();  

  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_retracted_deg);
  Serial.print(15); Serial.print(" "); Serial.println(ts);  
  delay(250);
  servo_retract.detach();  
  
  Serial.print(0); Serial.print(" "); Serial.println(ts);    // print end of session                 
  while(1){}                               //  Stops executing the program
}

  
 
