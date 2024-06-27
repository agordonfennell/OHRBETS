/*
-General notes--------------------------
* This program is a retractable spout training & pavlovian conditioning task
* Brake is engaged, spout is extended, and multi-spout head is rotated at start of session
* Set number of trials consisting of
  - tone (optional)
  - spout extension for set period of time then retraction (optional)
  - delivery of a defined number of solenoid openings
  - random inter-trial-interval
  - relative timing between tone and spout extension / liquid delivery set using tone_to_access_delay
* setup to accomodate setups with multiple spouts
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
  static byte trial_count = 60; 
 
  static boolean session_retract = 1;  // 0: no retractable spout, 1: retractable spout
  static boolean session_tone = 1;     // 0: no tone; 1: tone

  static byte lick_detection_circuit = 0; // 0: cap sensor, 1: voltage sensor
  static int tm_lick_latency_min = 50;

 // spout vectors (each element will correspond to a spout, add or remove elements based on system, default is setup for 5 spouts)
  static byte num_spouts = 5; // should have this many values for each of the following vectors
  
  static byte pinSol[] =                      {  4,  5,  6,  7,  8};
  static byte sol_duration[] =                { 30, 30, 30, 30, 30}; // calibrate to ~1.5µL / delivery
  static byte servo_radial_degs [] =          {  0, 30, 60, 90,120};
  static byte servo_retract_extended_degs[] = {180,180,180,180,180};
  static byte pinLickometer_ttl[] =           { 22, 22, 22, 22, 22};

  byte current_spout = 1; // set spout for session (1 to n, where n is the number of spouts on the system; used to index each vector above)

  
 // iti range (uniform random)
  static unsigned long min_iti = 10000; // minimum iti (ms) 
  static unsigned long max_iti = 30000; // maximum iti (ms)

 // access & solenoid --------
  static int access_time = 5000;          // length of time spout is extended each trial (ms)
  
  static byte num_sol = 5;                // number of solenoid opening access period
  
  int inter_sol_time = 180 - sol_duration[current_spout-1];        // interval between each solenoid opening (measured from end of previous until start of next)
 
 // tone cue -----------------
  static unsigned long tone_freq = 5000;  // (Hz)
  static int tone_duration = 2000;        // tone duration (ms)
  static int tone_to_access_delay = 3000; // delay from onset of tone to onset of access period (ms)

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinLickometer = 21; // only set to output if lick_detection_circuit = 1
  
 // outputs ---------------------------
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;
  static byte pinSpeaker = 12;  

  // ttls for external time stamps
  static byte pinSol_ttl = 23;
  static byte pinTone_ttl = 24;

//capacitance sensor variables
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;
  // note: code is setup to use sensor position 1

// servo brake variables / parameters ------------------------------------------------------
  Servo servo_brake;  // create servo object to control a servo
  static byte servo_brake_engaged_deg = 15;
  static byte servo_brake_disengaged_deg = 0;
  static int detach_servo_brake_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_retracted_deg = 140;
  unsigned long detach_servo_retract_ts = 0;
  static int detach_servo_retract_step = 200; // time in ms to allow for the servo to travel
  unsigned long ts_servo_retract_retracted;

// servo radial variable / parameters-------------------------------------------------------
  Servo servo_radial;

// flags -------------------------------------------------------------------------------------
 // variables---
  boolean solOpen;
  boolean frame;
  volatile byte lick;
  boolean lick_gate = 1;
  boolean first_loop = 1;
  boolean pinLickometer_state;
  boolean pinLickometer_state_previous;
  
// counters ---------------------------------------------------------------------------------
 // variables---
  volatile byte count_sol;                 // counter for number of solenoid openings per access period
  volatile byte count_trial;               // counter for number of trials

// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile unsigned long ts; 
  unsigned long iti;
  unsigned long ts_start;
  unsigned long ts_access_start; 
  unsigned long ts_tone_start; 
  unsigned long ts_sol_offset;
  unsigned long ts_sol_onset;
  unsigned long ts_sol_ttl_off;
  unsigned long ts_tone_ttl_off;
  unsigned long ts_lickometer_ttl_off; 
  unsigned long ts_lick_gate_open = 0;
  boolean flag_access_complete;
  boolean flag_tone_complete;

  volatile unsigned long brake_spout_delay; // random delay length between brake and access period
  
  unsigned long session_end_ts;

 // parameters---
  static int ttl_duration = 5; // duration of ttl pulses for external time stamps (ms)
  

// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); 

 // define inputs
  if(lick_detection_circuit == 1){
    pinMode(pinLickometer, INPUT);
  }

 // define outputs
  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinTone_ttl, OUTPUT);
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 
  pinMode(pinSpeaker, OUTPUT);

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
  Serial.print(130); Serial.print(" "); Serial.println(ts);   // print radial position moved
  Serial.print(127); Serial.print(" "); Serial.println(ts);
  Serial.print(127); Serial.print(" "); Serial.println(current_spout);
  delay(250);
  servo_radial.detach();

 // fully extend spout prior to session start
  servo_retract.attach(pinServo_retract); 
  Serial.print(13); Serial.print(" "); Serial.println(ts);      
  servo_retract.write(servo_retract_extended_degs[current_spout-1]);
  delay(250);
  servo_retract.detach();

  // setup capacitive touch sensor ---------------
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

 // save start time and send ttl to initiate scope
  ts_start=millis();  
}


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
// background functions (run constantly independent of task events)--------------------------------------------
 // generate timestamp ---------------------------
  ts=millis()-ts_start;

 // open lick gate -------------------------------
  if (ts > ts_lick_gate_open && ts_lick_gate_open != 0) {
    lick_gate = 1;
    ts_lick_gate_open = 0;
  }

 // detach spout servo ---------------------------
  if(ts >= detach_servo_retract_ts && detach_servo_retract_ts!=0){
    servo_retract.detach();
    detach_servo_retract_ts = 0;
  }
  
 // close solenoids---------------------------
  if (ts >= ts_sol_offset && ts_sol_offset != 0) {           // if time is after solenoid offset time
    digitalWrite(pinSol[current_spout - 1], LOW);              // set state to low
    Serial.print(14); Serial.print(" "); Serial.println(ts); // print sol offset
    ts_sol_offset = 0;                                       // reset solenoid offset time to close if statement
  }

 // turn off ttls for external time stamps ------------------------
 // lick---
  if(ts>=ts_lickometer_ttl_off && ts_lickometer_ttl_off!=0){
    digitalWrite(pinLickometer_ttl[current_spout-1],LOW); // write ttl low
    ts_lickometer_ttl_off = 0;            // reset off time to close if statement
  }
  
 // solenoid---
  if(ts>=ts_sol_ttl_off && ts_sol_ttl_off!=0){
    digitalWrite(pinSol_ttl,LOW);  // write ttl low
    ts_sol_ttl_off = 0;            // reset off time to close if statement
  }

 // tone---
  if(ts>=ts_tone_ttl_off && ts_tone_ttl_off!=0){
    digitalWrite(pinTone_ttl,LOW);  // write ttl low
    ts_sol_ttl_off = 0;            // reset off time to close if statement
  }
  
// session initialization (runs once at start) -----------------------------------------------------------------
  if(first_loop){
    Serial.print(1);   Serial.print(" "); Serial.println(ts);                              delay(3); // print start session
    Serial.print(127); Serial.print(" "); Serial.println(trial_count);                     delay(3); // print session duration
    Serial.print(104); Serial.print(" "); Serial.println(session_retract);                 delay(3); // print session_retract
    Serial.print(137); Serial.print(" "); Serial.println(session_retract);                 delay(3); // print session_tone
    Serial.print(128); Serial.print(" "); Serial.println(min_iti);                         delay(3); // print min_delay
    Serial.print(129); Serial.print(" "); Serial.println(max_iti);                         delay(3); // print max_delay
    Serial.print(115); Serial.print(" "); Serial.println(access_time);                     delay(3); // print access_time
    Serial.print(116); Serial.print(" "); Serial.println(sol_duration[current_spout - 1]); delay(3); // print sol_duration
    Serial.print(117); Serial.print(" "); Serial.println(num_sol);                         delay(3); // print num_sol
    Serial.print(118); Serial.print(" "); Serial.println(inter_sol_time);                  delay(3); // print inter_sol_time
    Serial.print(121); Serial.print(" "); Serial.println(tone_freq);                       delay(3); // print tone_fr
    Serial.print(122); Serial.print(" "); Serial.println(tone_duration);                   delay(3); // print tone_duration
    Serial.print(138); Serial.print(" "); Serial.println(tone_to_access_delay);            delay(3); // tone_to_access_delay

   // retract spout
    if(session_retract){ 
      servo_retract.attach(pinServo_retract); 
      Serial.print(15); Serial.print(" "); Serial.println(ts);      
      servo_retract.write(servo_retract_retracted_deg);
    }
    
   // allow time for servo travel and detach
    delay(250); 
    servo_retract.detach();

   // set time for first delivery
    iti = random(min_iti, max_iti);
    ts_access_start = ts + iti + tone_to_access_delay;
    ts_tone_start = ts + iti;
    
   first_loop = 0;

  }

// licking-----------------------------------------------------------------------------------------------------
 if(lick_detection_circuit == 0){ // if using cap sensor --------------------------------------------------------------
   // check state of sensor to see if it is currently touched
    currtouched = cap.touched(); 
  
    // check to see if touch onset occurred
    if ((currtouched & _BV(current_spout)) && !(lasttouched & _BV(current_spout)) ) { // if touched now but not previously
      lick = current_spout;                                                           // flag lick
    }
  
   // save current state for comparison with next state
    lasttouched = currtouched; 
 }

 if(lick_detection_circuit == 1){ // if using voltage sensor ----------------------------------------------------------
  // check state of sensor to see if it is currently touched
   pinLickometer_state = digitalRead(pinLickometer);

  // check to see if touch onset occurred
   if(pinLickometer_state > pinLickometer_state_previous) { // if touched now but not previously
        lick = current_spout+1;                             // flag lick
   }
   
  // save current state for next loop 
   pinLickometer_state_previous = pinLickometer_state;
 }

 // programmed consequences to licking
  if (lick > 0) { // if lick has occured
    if(lick_gate){
        Serial.print(30 + current_spout); Serial.print(" "); Serial.println(ts); // print lick onset
        digitalWrite(pinLickometer_ttl[current_spout-1],HIGH);                    // turn on ttl for external ts
        ts_lickometer_ttl_off = ts + ttl_duration;                // set ttl offset time

        lick_gate = 0; // close lick gate until tm_lick_latency_min ms have passed
        
        if (tm_lick_latency_min > sol_duration[lick - 1]) {
          ts_lick_gate_open = ts + tm_lick_latency_min;
        } else {
          ts_lick_gate_open = ts +  sol_duration[lick - 1];
        }

        
    }
    
    lick=0; // reset lick flag to close if statement
  }

 // schedule of magazine training --------------------------------------------------------------
 // start tone --------------------
 if(ts >= ts_tone_start && ts_tone_start != 0){
    if(session_tone){
      tone(pinSpeaker, tone_freq, tone_duration);  
      Serial.print(51); Serial.print(" "); Serial.println(ts); // print tone onset

      digitalWrite(pinTone_ttl,HIGH);       // set pin for ttl marker to high
      ts_sol_ttl_off = ts + tone_duration; // set time for ttl marker to turn off
    }
    flag_tone_complete = 1;
    ts_tone_start = 0;
 }

 
 // start access period------------
  if(ts >= ts_access_start && ts_access_start != 0){
    if(session_retract){fun_servo_retract_extended();}  // extend spout
    ts_servo_retract_retracted = ts + access_time;      // set time for retract spout
    ts_sol_onset = ts;                                  // set solenoid onset time to current time
    ts_access_start = 0;
  }

 // open solenoid num_sol times----
  if(ts >= ts_sol_onset && ts_sol_onset != 0 &&  count_sol < num_sol){
    digitalWrite(pinSol[current_spout - 1],HIGH);          // open solenoid
    ts_sol_offset = ts + sol_duration[current_spout - 1];  // set solenoid close time
    
    Serial.print(18); Serial.print(" "); Serial.println(ts); // print sol onset
    
    digitalWrite(pinSol_ttl,HIGH);      // turn on ttl for solenoid onset 
    ts_sol_ttl_off = ts + ttl_duration; // set ttl offset time

    ts_sol_onset = ts + inter_sol_time + sol_duration[current_spout - 1]; // set next solenoid onset time
    count_sol++;                                       // increase counter for solenoid
  }

 // end access period--------------
  if(ts >= ts_servo_retract_retracted && ts_servo_retract_retracted != 0){
    if(session_retract){fun_servo_retract_retracted();} // retract spout
    ts_servo_retract_retracted = 0;                     // reset retract time to close if statement

   // reset various variables and counters
    ts_sol_onset = 0;
    count_sol = 0;

    flag_access_complete = 1;
    count_trial++;
  }

 // setup start time for next trial-----
  if(flag_access_complete == 1 && flag_tone_complete == 1){
    iti = random(min_iti, max_iti);
    ts_access_start = ts + iti + tone_to_access_delay;
    ts_tone_start = ts + iti;
    
    flag_access_complete = 0;
    flag_tone_complete = 0;
  }
      
 // session termination ---------------------------------------------------------------------------------------------------
 // end session after trials exceed trial_count
  if(count_trial >= trial_count && session_end_ts == 0){
    session_end_ts = ts + min_iti-1000; // set end of session to 1s shorter than minimum ITI
  }
  
  if(ts > session_end_ts && session_end_ts != 0){
    fun_end_session();
    Serial.print(99); Serial.print(" "); Serial.println(ts);    // print end of session  
  }
}


// _______________________________________________________________________________________________________________________________________________________________________________
/// functions & interrupts_________________________________________________________________________________________________________________________________________________________

// servos ----------------------------------------------------------------
void fun_servo_retract_extended(){ //-------------------------------------
  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_extended_degs[current_spout-1]);
  Serial.print(13); Serial.print(" "); Serial.println(ts);            
  detach_servo_retract_ts = ts + detach_servo_retract_step;
}

void fun_servo_retract_retracted(){ //------------------------------------
  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_retracted_deg);
  Serial.print(15); Serial.print(" "); Serial.println(ts);            
  detach_servo_retract_ts = ts + detach_servo_retract_step;
}

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
  while(1){}                                                 // Stops executing the program
}

  
 
