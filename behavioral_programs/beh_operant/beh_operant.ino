/*
-General notes--------------------------
* This program is an operant task with multiple schedule and reinforcement options
* The ratio or reinforcement can be either fixed or progressive
* The reinforcer can be either liquid reward (1 or more solenoid openings) or extTTL stimulation (on or off)
* This program includes an optional brake control to limit the subjects ability to make operant responses
* This program includes an optional retractable spout to limit the subjects ability to make consumitory responses
* Solenoid sequence onset starts with spout extension
* access period and solenoid  opening are independent
  * check to ensure that the access period is longer than (sol_duration + inter_sol_time) x num_sol
* tone and access periods are on independent timing
* All events (except dynamic parameters) that occur during the arduino program are transmitted via serial with an id and time stamp in ms (2 columns)
  * If you adapt this program and add new events, make sure to follow the 2 column approach to avoid conflicts with
    existing programs
  * Dynamic parameters are represented by 2 rows, the first row is the time stamp for the parameter, and the second is the current parameter
    the column will always contain the id for the dynamic parameter
    - it is critical that the order is maintaied

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
  static unsigned long session_duration = 1800000; // session duration (ms) (note: avoid using formula here)
  static boolean session_brake = 1;            // 0: no brake during access period, 1: brake engaged during access period
  static boolean inactive_brake = 1;           // 0: no brake following inactive response, 1: brake for equivalent time to active response
  static boolean session_retract = 1;          // 0: no retractable spout, 1: retractable spout
  boolean session_contingency_current = 0;     // 0: right = active, 1: left = active
  boolean session_reinforcer_availability = 0; // 0: availible, 1: not availible
  static boolean session_reinforcer = 0;       // 0: liquid, 1: ttl out (laser, or other external hardware)
  static boolean session_setback = 1;          // 0: no setback, 1: setback to zero, 2: setback to negative
  static boolean extTTL_posneg = 0;            // 0: pos reinforcement (resp->on), 1: neg reinforcement (resp->off)

 // spout vectors (each element will correspond to a spout, add or remove elements based on system, default is setup for 5 spouts)
  static byte num_spouts = 5; // should have this many values for each of the following vectors
  
  static byte pinSol[] =                      {  4,  5,  6,  7,  8};
  static byte sol_duration[] =                { 30, 30, 30, 30, 30}; // calibrate to ~1.5µL / delivery
  static byte servo_radial_degs [] =          {  0, 30, 60, 90,120};
  static byte servo_retract_extended_degs[] = {180,180,180,180,180};
  static byte pinLickometer_ttl[] =           { 22, 22, 22, 22, 22};

  byte current_spout = 1; // set spout for session (1 to n, where n is the nubmer of spouts on the system; used to index each vector above)

 // external TTL vecotrs (each element will correspond to a TTL output, see format above)
  static byte num_extTTL = 3;
  static byte pinExtTTL[] = {26, 27, 28, 29};
  static byte current_extTTL = 1; //  set TTL for session

// switch parameters ---------
  static long tm_switch_contingency_step = -1;             // switch contingency every x ms (-1: no switching)
  static long tm_switch_reinforcer_availability_step = -1; // switch between reinforcer being available or witheld every x ms (-1: no switching)

 // rotation & schedule ------
  static int rotary_resoltion = 32; // number of rotation pulses per down sampled rotation pulse(raw PPR = 1024, rotary resolution of 16 -> 64 down sampled pulses per rotation (1024 / 16 = 64))
  static boolean schedule = 0;      // 0: fixed ratio, 1: progressive ratio
  int current_ratio = 8;            // number of down sampled rotation pulses per reward (absolute rotation will depend on both current_ratio and rotary_resolution, e.g. rotary resolution of 16(64 down sampled pulses per rotaiton), and a current_ratio of 32 means 1/2 rotation per reward)
  
  static int pr_step = 8;                   // step in number of down sampled rotation pusles per reward
  static double pr_function = 1.25;         // 1: linear increase, >1: logorithmic (1.25 aproximates Richardson and Roberts (1996)  [5e(R*0.2)] - 5 )
  static unsigned long pr_timeout = 600000; // amount of time wihtout a response before session ends (ms)

 // pre access delay ---------
  static unsigned long brake_delay = 250;         // minimum delay from brake onset to access period start (allows for brake to fully engage)
  
  static unsigned long brake_to_access_delay_min = 3000; // minimum delay after brake engages before access period start
  static unsigned long brake_to_access_delay_max = 3000; // maximum delay after brake engages before access period start
 
 // access & solenoid --------
  static int access_time = 3000;          // total time before spout is extended
  static byte num_sol = 5;                // number of solenoid opening access period
  int inter_sol_time = 180 - sol_duration[current_spout-1];        // interval between each solenoid opening (measured from end of previous until start of next)
  
 // opto --------------------
  static int extTTL_duration = 2000; // duration of ttl high (pos) or ttl low (neg)

 // additional time out -----
  static int duration_additional_to = 3000; // time out period (ms) that extends beyond access/opto period
 
 // tone cue -----------------
  static unsigned long tone_freq = 5000;  // tone played during access period / extTTL period (Hz)
  static int tone_duration = 2000;        // tone duration (ms)
  
  static unsigned long brake_to_tone_delay_min = 0;   // minimum delay after brake engages before tone start
  static unsigned long brake_to_tone_delay_max = 0;   // maximum delay after brake engages before tone start
  

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinRotaryEncoderA = 3; 
  static byte pinRotaryEncoderB = 2; 
  
 // outputs ---------------------------
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;
  static byte pinSpeaker = 12;

  // ttls for external time stamps
  static byte pinSol_ttl = 52;
  static byte pinRotation_active_ttl = 50;
  static byte pinRotation_inactive_ttl = 48;

//capicitance sensor variables
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;
  // note: code is setup to use sensor position 1

// servo brake variables / parameters ------------------------------------------------------
  Servo servo_brake;  // create servo object to control a servo
  static byte servo_brake_engaged_deg = 15;
  static byte servo_brake_disengaged_deg = 0;
  unsigned long detach_servo_brake_ts = 0;
  static int detach_servo_brake_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_retracted_deg = 120;
  unsigned long detach_servo_retract_ts = 0;
  static int detach_servo_retract_step = 100; // time in ms to allow for the servo to travel
  unsigned long ts_servo_retract_retracted;

// servo radial varialbe / parameters-------------------------------------------------------
  Servo servo_radial;
  
// rotary encoder variables / parameters ----------------------------------------------------
 // variables---
  volatile boolean rotation_right_flag = 0; // interupt flack for rightward rotation
  volatile boolean rotation_left_flag = 0;  // interupt flack for leftward  rotation
  
  volatile int rotation_active_counter_trial = 0;   // full resolution rotation counter
  volatile int rotation_inactive_counter_trial = 0; // full resolution rotation counter
  
  volatile int rotation_active = 0;   // downsampled rotation counter
  volatile int rotation_inactive = 0; // downsampled rotation counter

  volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderA to signal that the encoder has arrived at a detent
  volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
  volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
  
  uint8_t maskA;  // mask for reading pin A
  uint8_t maskB;  // mask for reading pin B
  uint8_t maskAB; // combined mask for pin A and B
  volatile uint8_t *port;
  
// flags -------------------------------------------------------------------------------------
 // variables---
  boolean solOpen;
  boolean frame;
  boolean lick;
  boolean rotation_gate = 1; // used to gate rotation relative to brake engagement

// counters ---------------------------------------------------------------------------------
 // variables---
  volatile byte count_sol;                 // counter for number of solenoid openings per access period
  unsigned long pr_step_current = pr_step; // current step increase (remains stable if using linear pr_function)

// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile unsigned long ts; 
  unsigned long ts_start;
  unsigned long ts_access_start; 
  unsigned long ts_tone_start;
  unsigned long ts_sol_offset;
  unsigned long ts_sol_onset;
  unsigned long ts_sol_ttl_off;
  unsigned long ts_extTTL_offset;
  unsigned long ts_lickomter_ttl_off; 
  unsigned long ts_rotation_active_ttl_off = 0;
  unsigned long ts_rotation_inactive_ttl_off = 0;
  unsigned long ts_to_end;
  volatile unsigned long ts_pr_timeout;

  static unsigned long tm_switch_contingency;         
  static unsigned long tm_switch_reinforcer_availability; 

  volatile unsigned long brake_access_delay; // random delay length between brake and access period
  volatile unsigned long brake_tone_delay;   // random delay length between brake and tone
  
  unsigned long session_end_ts;

 // parameters---
  static int ttl_duration = 5; // duration of tttl pulses for external time stamps (ms)
  

// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); 

 // define inputs --------------------------------
  pinMode(pinRotaryEncoderA,INPUT_PULLUP);
  pinMode(pinRotaryEncoderB,INPUT_PULLUP);
  
 // define outputs
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 
  
  for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each solenoid
    pinMode(pinSol[i_sol], OUTPUT);                       // define sol as output
    pinMode(pinLickometer_ttl[i_sol], OUTPUT);           // 
  } 
  
  for(uint8_t i_extTTL = 0; i_extTTL < num_extTTL; i_extTTL++){
    pinMode(pinExtTTL[i_extTTL], OUTPUT);
  }

  pinMode(pinSpeaker, OUTPUT);
  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinRotation_active_ttl, OUTPUT);
  pinMode(pinRotation_inactive_ttl, OUTPUT);

 // rotary encoder ------------------------------
 // setup interupts for the rotary encoder 
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderB),fpinRotaryEncoderB,RISING); // interrupt on pinRotaryEncoderA
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderA),fpinRotaryEncoderA,RISING); // interrupt on pinRotaryEncoderB
  
 // save starting state of rotary encoder
  maskA = digitalPinToBitMask(pinRotaryEncoderA);
  maskB = digitalPinToBitMask(pinRotaryEncoderB);
  maskAB = maskA | maskB;
  port = portInputRegister(digitalPinToPort(pinRotaryEncoderA));

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

  // setup capative touch sesnsor ---------------
  if (!cap.begin(0x5A)) {                   // if the sensor is not detected
    Serial.println("MPR121 not detected!"); // print warning (and crash python program)
    while (1);
  }

  cap.setThresholds(6,2);
  delay(50);
  Serial.print(998);   Serial.print(" "); Serial.println(cap.filteredData(1));

 // wait for serial command before initating session---------------------------------------------------------------------
  while (Serial.available() <= 0) {} // wait for serial input to start session
  delay(100);

 // save start time and send ttl to initate scope
  ts_start=millis();  
}


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
// background functions (run constantly independent of task events)--------------------------------------------
 // generate timestamp ---------------------------
  ts=millis()-ts_start;

 // detach brake servo ---------------------------
  if(ts >= detach_servo_brake_ts && detach_servo_brake_ts!=0){
    servo_brake.detach();
    detach_servo_brake_ts = 0;
  }

 // detach spout servo ---------------------------
  if(ts >= detach_servo_retract_ts && detach_servo_retract_ts!=0){
    servo_retract.detach();
    detach_servo_retract_ts = 0;
  }

 // close solenoids---------------------------
  if(ts>=ts_sol_offset && ts_sol_offset!=0){                 // if time is after solenoid offset time
    digitalWrite(pinSol[current_spout - 1], LOW);                               // close solenoid
    Serial.print(14); Serial.print(" "); Serial.println(ts); // print sol offset
    ts_sol_offset = 0;    // reset solenoid offset time to close if statement
    }

 // turn off ttls for external time stamps ------------------------
 // lick---
  if(ts>=ts_lickomter_ttl_off && ts_lickomter_ttl_off!=0){
    digitalWrite(pinLickometer_ttl[current_spout - 1],LOW); // write ttl low
    ts_lickomter_ttl_off = 0;            // reset off time to close if statement
  }
  
 // solenoid---
  if(ts>=ts_sol_ttl_off && ts_sol_ttl_off!=0){
    digitalWrite(pinSol_ttl,LOW);  // write ttl low
    ts_sol_ttl_off = 0;            // reset off time to close if statement
  }

 // rotation active---
  if(ts>=ts_rotation_active_ttl_off && ts_rotation_active_ttl_off!=0){
    digitalWrite(pinRotation_active_ttl,LOW); // write ttl low
    ts_rotation_active_ttl_off = 0;           // reset off time to close if statement
  }

 // rotation inactive---
  if(ts>=ts_rotation_inactive_ttl_off && ts_rotation_inactive_ttl_off!=0){
    digitalWrite(pinRotation_inactive_ttl,LOW); // write ttl low
    ts_rotation_inactive_ttl_off = 0;           // reset off time to close if statement
  }
  
// session initialization (runs once at start) -----------------------------------------------------------------
  if(session_end_ts == 0){
    Serial.print(1);   Serial.print(" "); Serial.println(ts);                          delay(3);// print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);            delay(3);// print session duration
    Serial.print(101); Serial.print(" "); Serial.println(rotary_resoltion);            delay(3);// print rotary_resoltion
    Serial.print(103); Serial.print(" "); Serial.println(session_brake);               delay(3);// print session_brake
    Serial.print(104); Serial.print(" "); Serial.println(session_retract);             delay(3);// print session_retract
    Serial.print(107); Serial.print(" "); Serial.println(session_reinforcer);          delay(3);// print session_reinforcer
    Serial.print(108); Serial.print(" "); Serial.println(extTTL_posneg);               delay(3);// print extTTL_posneg
    Serial.print(109); Serial.print(" "); Serial.println(schedule);                    delay(3);// print schedule
    Serial.print(110); Serial.print(" "); Serial.println(pr_step);                     delay(3);// print pr_step
    Serial.print(111); Serial.print(" "); Serial.println(pr_function);                 delay(3);// print pr_function
    Serial.print(112); Serial.print(" "); Serial.println(brake_delay);                 delay(3);// print brake_delay
    Serial.print(113); Serial.print(" "); Serial.println(brake_to_access_delay_min);   delay(3);// print brake_to_access_delay_min
    Serial.print(114); Serial.print(" "); Serial.println(brake_to_access_delay_max);   delay(3);// print brake_to_access_delay_max
    Serial.print(140); Serial.print(" "); Serial.println(brake_to_tone_delay_min);     delay(3);// print brake_to_tone_delay_min
    Serial.print(141); Serial.print(" "); Serial.println(brake_to_tone_delay_max);     delay(3);// print brake_to_tone_delay_max
    Serial.print(115); Serial.print(" "); Serial.println(access_time);                 delay(3);// print access_time
    Serial.print(117); Serial.print(" "); Serial.println(num_sol);                     delay(3);// print num_sol
    Serial.print(118); Serial.print(" "); Serial.println(inter_sol_time);              delay(3);// print inter_sol_time
    Serial.print(119); Serial.print(" "); Serial.println(extTTL_duration);             delay(3);// print extTTL_duration
    Serial.print(120); Serial.print(" "); Serial.println(duration_additional_to);      delay(3);// print duration_
    Serial.print(121); Serial.print(" "); Serial.println(tone_freq);                   delay(3);// print tone_fr
    Serial.print(122); Serial.print(" "); Serial.println(tone_duration);               delay(3);// print tone_duration
    Serial.print(124); Serial.print(" "); Serial.println(tm_switch_contingency_step);               delay(3);// print tm_switch_contingency_step
    Serial.print(125); Serial.print(" "); Serial.println(tm_switch_reinforcer_availability_step);   delay(3);// print tm_switch_reinforcer_availability_step
    Serial.print(126); Serial.print(" "); Serial.println(session_setback);             delay(3);// print session_setback
    
   // disengage brake
    servo_brake.attach(pinServo_brake); 
    Serial.print(12); Serial.print(" "); Serial.println(ts);      
    servo_brake.write(servo_brake_disengaged_deg);
    delay(250); 
    servo_brake.detach();
    
   // retract spout
    if(session_retract){ 
      servo_retract.attach(pinServo_retract); 
      Serial.print(15); Serial.print(" "); Serial.println(ts);      
      servo_retract.write(servo_retract_retracted_deg);
    }
    delay(250); 
    servo_retract.detach();

    if(session_reinforcer == 1 && extTTL_posneg == 1){ // if extTTL reinforcer and negative reinforcement
      digitalWrite(pinExtTTL[current_extTTL - 1], HIGH);
      Serial.print(16); Serial.print(" "); Serial.println(ts);    // print extTTL onset
      }

    Serial.print(102);   Serial.print(" "); Serial.println(ts);             // print time for ratio (ts must precede value)
    Serial.print(102);   Serial.print(" "); Serial.println(current_ratio);  // print current ratio

    Serial.print(106); Serial.print(" "); Serial.println(ts);                            // print time for session_contingency_current (ts must precede value)
    Serial.print(106); Serial.print(" "); Serial.println(session_contingency_current);   // print session_contingency_current

    Serial.print(123); Serial.print(" "); Serial.println(ts);                              // print time for session_reinforcer_availability (ts must precede value)
    Serial.print(123); Serial.print(" "); Serial.println(session_reinforcer_availability); // print session_reinforcer_availability

   // calculate time for next switches
   if(tm_switch_contingency_step != -1){
    tm_switch_contingency = ts + tm_switch_contingency_step;
   }

   if(tm_switch_reinforcer_availability_step != -1){
    tm_switch_reinforcer_availability = ts + tm_switch_reinforcer_availability_step;
   }

   if(schedule==1){                     // if progressive ratio
    ts_pr_timeout = ts + pr_timeout;    // set timeout time current time + timeout duration
   }

    session_end_ts = ts + session_duration; // set session end time 
  }

// parameter switches ------------------------------------------------------------------------------
 // contingency (reinforced direction) -------------------------------------------------------------
  if(ts >= tm_switch_contingency && tm_switch_contingency_step != -1){
    session_contingency_current = !session_contingency_current; // invert session_contingency_cur
    tm_switch_contingency = ts + tm_switch_contingency_step;         // set next switch time

    Serial.print(106); Serial.print(" "); Serial.println(ts);                            // print time for session_contingency_current (ts must precede value)
    Serial.print(106); Serial.print(" "); Serial.println(session_contingency_current);   // print session_contingency_current
  }

 // reinforcer availability ------------------------------------------------------------------------
  if(ts > tm_switch_reinforcer_availability && tm_switch_reinforcer_availability_step != -1){
    session_reinforcer_availability = !session_reinforcer_availability; // invert session_reinforcer_availability
    tm_switch_reinforcer_availability = ts + tm_switch_reinforcer_availability_step;

    Serial.print(123); Serial.print(" "); Serial.println(ts);                              // print time for session_reinforcer_availability (ts must precede value)
    Serial.print(123); Serial.print(" "); Serial.println(session_reinforcer_availability); // print session_reinforcer_availability
  }

// licking-----------------------------------------------------------------------------------------------------
 // check state of sensor to see if it is currently touched
  currtouched = cap.touched(); 

  // check to see if touch onset occured
  if ((currtouched & _BV(current_spout)) && !(lasttouched & _BV(current_spout)) ) { // if touched now but not previously
    lick = current_spout;
  }

 // save current state for comparision with next state
  lasttouched = currtouched; 

 // programed consequences to licking
  if (lick > 0) { // if lick has occured
      Serial.print(30 + current_spout); Serial.print(" "); Serial.println(ts); // print lick onset
      digitalWrite(pinLickometer_ttl[current_spout-1],HIGH);                    // turn on ttl for external ts
      ts_lickomter_ttl_off = ts + ttl_duration;                // set ttl offset time

      lick=0; // reset lick flag to close if statement
  }

// rotary encoder ---------------------------------------------------------------------------------------------
 // right rotation -----------------------------------------------------------------------------
   if(rotation_right_flag){
    if(session_contingency_current == 0){ // if right = active
      rotation_active_counter_trial++;      // increase active counter 
    } else {                              // if left  = active
      rotation_inactive_counter_trial++;    // increase inactive counter 
    }
    rotation_right_flag = 0; // reset right flag to close if statement
   }

 // left rotation ------------------------------------------------------------------------------
   if(rotation_left_flag){
    if(session_contingency_current == 0){ // if right = active
      rotation_inactive_counter_trial++;  // increase inactive counter 
    } else {                              // if left  = active
      rotation_active_counter_trial++;    // increase active counter 
    }
    rotation_left_flag = 0; // reset left flag to close if statement
   }
   
 // active rotation ----------------------------------------------------------------------------
   if(rotation_active_counter_trial >= rotary_resoltion){ // down sample rotation counter
    if(rotation_gate && session_reinforcer_availability == 0){
      rotation_active++; // increase active counter
      
      // subtract opposite direction, but not below 0
      if(session_setback>0){
        if(session_setback==1 && rotation_inactive > 0){
          rotation_inactive--;
        }

        if(session_setback==2){
          rotation_inactive--;
        }  
      }
      
     Serial.print(81); Serial.print(" "); Serial.println(ts); // print active rotation reaching criteria
      
      digitalWrite(pinRotation_active_ttl,HIGH);              // turn on ttl for active criteria
      ts_rotation_active_ttl_off = ts + ttl_duration;         // set ttl offset time
      
      } else { // if rotation gate is closed (during access period or time out)
        
        Serial.print(83); Serial.print(" "); Serial.println(ts); // print active rotation reaching criteria during brakeed or non reinforced periods
        
      }
      rotation_active_counter_trial = 0; // reset rotation counter to close if statement
   }

 // inactive rotation --------------------------------------------------------------------------
   if(rotation_inactive_counter_trial >= rotary_resoltion){ // down sample rotation counter
    if(rotation_gate && session_reinforcer_availability == 0){
      rotation_inactive++;
  
     // subtract opposite direction, but not below 0
      if(session_setback>0){
        if(rotation_active > 0){
          
          if(session_setback==1 && rotation_active > 0){
            rotation_active--;
          }

          if(session_setback==2){
            rotation_active--;
          }
          
          }
      }
      Serial.print(71); Serial.print(" "); Serial.println(ts); // print inactive rotation reaching criteria
  
      digitalWrite(pinRotation_inactive_ttl,HIGH);              // turn on ttl for inactive criteria
      ts_rotation_inactive_ttl_off = ts + ttl_duration;         // set ttl offset time
      
     } else { // if rotation gate is closed (during access period or time out)
      
      Serial.print(73); Serial.print(" "); Serial.println(ts); // print inactive rotation during brakeed or non reinforced periods
     }
     rotation_inactive_counter_trial = 0; // reset rotation counter to close if statement
   }

 // schedule of reinforcement ------------------------------------------------------------------
 // liquid reinforcer ---------------------------------------------------------------
  if(session_reinforcer == 0){ // if using liquid as reinforcer and reinforcer is available
  if (rotation_active >= current_ratio) { // once down sampled rotation count reaches current_ratio

      // initialize access period-----
      if(ts_access_start == 0 && ts_to_end == 0){ 
        Serial.print(82); Serial.print(" "); Serial.println(ts);    // print reached criteria
        if(session_brake){fun_servo_brake_engage();}                // engage brake
        rotation_gate = 0;                                          // close rotation gate
        
        brake_access_delay = random(brake_to_access_delay_min, brake_to_access_delay_max);         // calculate random delay
        ts_access_start = ts + brake_delay + brake_access_delay; // set access start time 
        
        brake_tone_delay = random(brake_to_tone_delay_min, brake_to_tone_delay_max);           // calculate random delay
        ts_tone_start = ts + brake_delay + brake_tone_delay; // set access start time 
        
      // increase ratio if pr
      if(schedule == 1){ // if pr
        current_ratio = current_ratio + pr_step_current;  // increase current ratio by current step
        rotation_active = current_ratio;                  // increase active rotation to prevent stopping trial

        ts_pr_timeout = ts + pr_timeout;    // set timeout time current time + timeout duration

        Serial.print(102);   Serial.print(" "); Serial.println(ts);             // print time for ratio
        Serial.print(102);   Serial.print(" "); Serial.println(current_ratio);  // print current ratio
        
        if(pr_function > 1){ // if logorithmic increase
          pr_step_current = round(pr_step_current*pr_function);   // increase current setp and round
          }
        }
      } 

     // start tone---------------------
      if(ts >= ts_tone_start && ts_tone_start != 0){
        tone(pinSpeaker, tone_freq, tone_duration); 
        Serial.print(51); Serial.print(" "); Serial.println(ts); // print tone onset
        ts_tone_start = 0;
      }
      
     // start access period------------
      if(ts >= ts_access_start && ts_access_start != 0){
        if(session_retract){fun_servo_retract_extended();}  // extend spout
        ts_servo_retract_retracted = ts + access_time;      // set time for retract spout
        ts_to_end = ts_servo_retract_retracted + duration_additional_to; // set end of to time
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
      }

     // end time out------------------
      if(ts > ts_to_end && ts_to_end != 0){
       // reset various variables and counters
        ts_sol_onset = 0;
        count_sol = 0;
                
        rotation_active = 0;
        rotation_active_counter_trial = 0;
        rotation_inactive = 0;
        rotation_inactive_counter_trial = 0;

        if(session_brake){fun_servo_brake_disengage();}     // disengage brake 
        rotation_gate = 1; // open rotation gate
        ts_to_end = 0;     // reset end of to time to close if statement
      }
      
    }
  }
  
 // extTTL reinforcer ---------------------------------------------------------------
  if(session_reinforcer == 1){ // if using extTTL as reinforcer and reinforcer is available
  if (rotation_active >= current_ratio) { // once down sampled rotation count reaches fixed ratio
     // initialize access period-----
      if(ts_access_start == 0 && ts_to_end == 0){ 
        if(session_brake){fun_servo_brake_engage();}                // engage brake
        rotation_gate = 0;                                          // close rotation gate
        Serial.print(82); Serial.print(" "); Serial.println(ts);    // print reached criteria
        
        brake_access_delay = random(brake_to_access_delay_min, brake_to_access_delay_max);           // calculate random delay
        ts_access_start = ts + brake_delay + brake_access_delay; // set access start time

        brake_tone_delay = random(brake_to_access_delay_min, brake_to_access_delay_max);           // calculate random delay
        ts_tone_start = ts + brake_delay + brake_tone_delay; // set access start time 
        
        // increase ratio if pr
        if(schedule == 1){ // if pr
          current_ratio = current_ratio + pr_step_current;  // increase current ratio by current step
          rotation_active = current_ratio;                  // increase active rotation to prevent stopping trial

          Serial.print(102);   Serial.print(" "); Serial.println(ts);             // print time for ratio
          Serial.print(102);   Serial.print(" "); Serial.println(current_ratio);  // print current ratio
          
          if(pr_function > 1){ // if logorithmic increase
            pr_step_current = round(pr_step_current*pr_function);   // increase current setp and round
            }
          }
      }

     // start tone---------------------
      if(ts >= ts_tone_start && ts_tone_start != 0){
        tone(pinSpeaker, tone_freq, tone_duration); 
        Serial.print(51); Serial.print(" "); Serial.println(ts); // print tone onset
        ts_tone_start = 0;
      }
      
     // start brakeed period and change extTTL state--
      if(ts >= ts_access_start && ts_access_start != 0){
        ts_access_start = 0;

        if(extTTL_posneg == 0){ // if positive reinforcement ----
          digitalWrite(pinExtTTL[current_extTTL - 1], HIGH);                               // turn on extTTL
          Serial.print(16); Serial.print(" "); Serial.println(ts);    // print extTTL onset
        }

        if(extTTL_posneg == 1){ // if negative reinforcement ----
          digitalWrite(pinExtTTL[current_extTTL - 1], LOW);                                // turn off extTTL
          Serial.print(17); Serial.print(" "); Serial.println(ts);    // print extTTL onset
        }

        ts_extTTL_offset = ts + extTTL_duration;                                  // set extTTL onset
        ts_to_end = ts_extTTL_offset + duration_additional_to;                   // set end of to time
      }

     // change extTTL state & end brakeed peroid---
      if(ts >= ts_extTTL_offset && ts_extTTL_offset != 0){
        ts_extTTL_offset = 0; 
        
         if(extTTL_posneg == 0){ // if positive reinforcement ----
          digitalWrite(pinExtTTL[current_extTTL - 1], LOW);                                // turn off extTTL
          Serial.print(17); Serial.print(" "); Serial.println(ts);    // print extTTL onset
        }

        if(extTTL_posneg == 1){ // if negative reinforcement ----
          digitalWrite(pinExtTTL[current_extTTL - 1], HIGH);                               // turn on extTTL
          Serial.print(16); Serial.print(" "); Serial.println(ts);    // print extTTL onset
        }
      }

     // end time out------------------
     if(ts >= ts_to_end && ts_to_end != 0){   
        rotation_active = 0;
        rotation_active_counter_trial = 0;
        rotation_inactive = 0;
        rotation_inactive_counter_trial = 0;

        if(session_brake){fun_servo_brake_disengage();}     // disengage brake  
        rotation_gate = 1;
        ts_to_end = 0;
      }
    }
  }

 // Inactive criteria --------------------------------------------------------------------------
  if (rotation_inactive >= current_ratio) {
    //brake inactive direction for as long as active direction but without tone or reward
      if(inactive_brake){
        if(ts_to_end == 0){
          rotation_gate = 0;                       // close rotation gate
          Serial.print(72); Serial.print(" "); Serial.println(ts); // print stim/tone onset
          fun_servo_brake_engage();                // engage brake
          ts_to_end = ts + brake_delay + access_time + duration_additional_to;
        }

        if(ts > ts_to_end && ts_to_end != 0){
         // reset various variables and counters
                  
          rotation_active = 0;
          rotation_active_counter_trial = 0;
          rotation_inactive = 0;
          rotation_inactive_counter_trial = 0;
  
          fun_servo_brake_disengage();     // disengage brake 
          rotation_gate = 1; // open rotation gate
          ts_to_end = 0;     // reset end of to time to close if statement
          rotation_inactive=0;
      }
        
      } else {
        Serial.print(72); Serial.print(" "); Serial.println(ts); // print stim/tone onset
        rotation_inactive=0;
      }
    }

 // session termination ---------------------------------------------------------------------------------------------------
 // end session if pr timeout is reached
  if(ts>ts_pr_timeout && ts_pr_timeout!=0){
    fun_end_session();
    Serial.print(98); Serial.print(" "); Serial.println(ts);    // print end of session  
  }

  // end session if end time is reached
  if(ts > session_end_ts && session_end_ts != 0){
    fun_end_session();
    Serial.print(99); Serial.print(" "); Serial.println(ts);    // print end of session  
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

// servos ----------------------------------------------------------------
void fun_servo_brake_engage(){ //-----------------------------------------
  servo_brake.attach(pinServo_brake);  
  servo_brake.write(servo_brake_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);            
  detach_servo_brake_ts = ts + detach_servo_brake_step;
}

void fun_servo_brake_disengage(){ //--------------------------------------
  servo_brake.attach(pinServo_brake);  
  servo_brake.write(servo_brake_disengaged_deg);
  Serial.print(12); Serial.print(" "); Serial.println(ts);            
  detach_servo_brake_ts = ts + detach_servo_brake_step;
}

void fun_servo_retract_extended(){ //-------------------------------------
  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_extended_degs[current_spout - 1]);
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
