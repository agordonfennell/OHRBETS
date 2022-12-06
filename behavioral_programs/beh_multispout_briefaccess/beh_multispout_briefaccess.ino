// dependencies
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Servo.h>
static byte num_prime = 2; // number of priming pulses at start of session


// session parameters *************************************************************************************************************

//session
static byte spout_block_set[] = {3,4,1,0,2,0,2,3,1,4,3,4,2,0,1,3,4,0,1,2,4,0,1,0,4,2,3,1,3,2,4,0,3,1,1,2,0,4,3,2,1,2,3,0,4,4,0,1,3,2,2,1,0,4,3,0,2,1,4,3,2,0,0,2,1,4,4,3,1,3,3,0,3,4,1,1,2,2,4,0,2,3,0,1,1,2,4,4,3,0,2,0,4,1,3,0,3,2,4,1};

static unsigned long access_time = 3000; // total time before spout is retracted
static unsigned long min_iti = 8000;     // minimum iti (ms)
static unsigned long max_iti = 13000;    // maximum iti (ms)

/*
// calibrate position
static byte spout_block_set[] = {4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,3,4,4,3,1};
static int access_time = 3000;          // total time before spout is retracted
static unsigned long min_iti = 3000;           // minimum iti (ms)
static unsigned long max_iti = 3000;           // maximum iti (ms)
*/

//---------------------------------------------------------------------------------------------------------------
unsigned long print_cap_step = 100;
unsigned long ts_print_cap_step;
static byte trial_count = 100;

static int tone_freq = 5000;

static byte num_spouts = 5; // todo: change to formula?
static byte pinSol[]       = { 4, 5,  6,  8,  9};
static byte sol_duration[] = {28, 34, 22,  38, 11}; //{28, 13, 18, 16, 42};
static byte pinLickometer_ttl[] = {24, 24, 24, 24, 24}; // dual box
static byte servo_radial_degs[] = {9, 36, 64, 92, 123};
static byte servo_retract_extended_degs[] = {175, 169, 168, 168, 169};
//static byte servo_retract_extended_degs[] = {0, 0, 0, 0, 0};
static byte sweep_deg = 77;
byte current_sol = pinSol[0];

static boolean access_type = 0; //0: free-acess (lick triggered solenoid throughout access period), 1: limited (set number of solenoid openings)

// general parameters -------

static boolean session_retract = 1;          // 0: no retractable spout, 1: retractable spout
static boolean session_serial_initiate = 1;  // 0: starts without serial input, 1: waits for serial input before starting

static byte num_sol = 5;                // number of solenoid opening access period
static int inter_sol_time = 180;        // interval between each solenoid opening (measured from end of previous until start of next)

byte current_spout = spout_block_set[0];

// arduino pins ----------------------------------------------------------------------------
// inputs ----------------------------
static byte pinLickometer = 6;

// outputs ---------------------------
static byte pinServo_retract = 37;
static byte pinServo_break = 39;
static byte pinServo_radial = 41;
static byte pinSpeaker = 12;

// ttls for external time stamps
static byte pinSol_ttl = 52;
//static byte pinTrial_ttl = 36; // single box
static byte pinTrial_ttl = 26; // dual box

boolean initiate_spout_seq = 0;
unsigned long move_servo_retract_retracted_ts;
static byte detach_buffer = 100; // aditional latency prior to next servo onset

//capicitance sensor variables
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// servo break variables / parameters ------------------------------------------------------
Servo servo_break;  // create servo object to control a servo
static byte servo_break_engaged_deg = 13;
static byte servo_break_disengaged_deg = servo_retract_extended_degs[0];
unsigned long detach_servo_break_ts = 0;
static int detach_servo_break_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
Servo servo_retract;
static byte servo_retract_retracted_deg = 120;
unsigned long detach_servo_retract_ts = 0;
static int detach_servo_retract_step = 100; // time in ms to allow for the servo to travel
unsigned long ts_servo_retract_retracted;

// servo rotary spout variables / parameters ------------------------------------------
Servo servo_radial;
unsigned long detach_servo_radial_ts = 0;
static int detach_servo_radial_step = 1000; // time in ms to allow for the servo to travel
unsigned long ts_servo_radial_traveled;
unsigned long move_servo_radial_sweep_ts;
unsigned long move_servo_radial_ts;

// flags -------------------------------------------------------------------------------------
// variables---
boolean solOpen;
boolean frame;
volatile byte lick;
boolean lick_gate = 0;
boolean first_loop = 1;
boolean toggle_spout_update;

// counters ---------------------------------------------------------------------------------
// variables---
volatile byte count_sol;                 // counter for number of solenoid openings per access period
volatile byte count_trial;               // counter for number of trials

// time stamps & timers ---------------------------------------------------------------------
// variables---
volatile unsigned long ts;
unsigned long ts_start;
unsigned long ts_access_start;
unsigned long ts_sol_offset;
unsigned long ts_sol_onset;
unsigned long ts_sol_ttl_off;
unsigned long ts_lickomter_ttl_off;

volatile unsigned long break_spout_delay; // random delay length between break and access period

unsigned long session_end_ts;

// parameters---
static int ttl_duration = 2; // duration of tttl pulses for external time stamps (ms)


// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  // define inputs --------------------------------
  pinMode(pinLickometer, INPUT);

  // define outputs
  pinMode(pinSpeaker, OUTPUT);
  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinTrial_ttl, OUTPUT);
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_break, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 
  
  
  for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each solenoid
    pinMode(pinSol[i_sol], OUTPUT);                       // define sol as output
    pinMode(pinLickometer_ttl[i_sol], OUTPUT);           // 
  } 

  // engage servo break prior to session start
  servo_break.attach(pinServo_break);
  Serial.print(11); Serial.print(" "); Serial.println(ts);
  servo_break.write(servo_break_engaged_deg);
  delay(100);
  servo_break.detach();

  // set initial spout position
  servo_radial.attach(pinServo_radial);
  Serial.print(127); Serial.print(" "); Serial.println(ts);
  Serial.print(127); Serial.print(" "); Serial.println(current_spout);
  servo_radial.write(servo_radial_degs[current_spout]);
  delay(500);
  servo_radial.detach();

  // fully extend spout prior to session start if using liquid reinforcer
  servo_retract.attach(pinServo_retract);
  Serial.print(13); Serial.print(" "); Serial.println(ts);
  servo_retract.write(servo_retract_extended_degs[current_spout]);
  delay(500);
  servo_retract.detach();
  
  // prime spouts
  if(num_prime > 0){
    for (uint8_t i_prime = 0; i_prime < num_prime; i_prime++){
      for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++){
        digitalWrite(pinSol[i_sol], HIGH);
        delay(sol_duration[i_sol]);
        digitalWrite(pinSol[i_sol], LOW);
        delay(100 - sol_duration[i_sol]);
      }
    }
  }

  // setup capative touch sesnsor ---------------
  if (!cap.begin(0x5A)) {                   // if the sensor is not detected
    Serial.println("MPR121 not detected!"); // print warning (and crash python program)
    while (1);
  }

  
  cap.setThresholds(6, 1);
  delay(50);
  Serial.print(998);   Serial.print(" "); Serial.println(cap.filteredData(1));

  // wait for serial command before initating session---------------------------------------------------------------------
  if (session_serial_initiate) {
    while (Serial.available() <= 0) {} // wait for serial input to start session
  }

  delay(50);

  // save start time and send ttl to initate scope
  ts_start = millis();
} // end setup


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
  // background functions (run constantly independent of task events)--------------------------------------------
  // generate timestamp ---------------------------
  ts = millis() - ts_start;


  // update spout (toggled below) -----------------
  spout_update();

  // detach break servo ---------------------------
  if (ts >= detach_servo_break_ts && detach_servo_break_ts != 0) {
    servo_break.detach();
    detach_servo_break_ts = 0;
  }

  // detach spout servo ---------------------------
  if (ts >= detach_servo_retract_ts && detach_servo_retract_ts != 0) {
    servo_retract.detach();
    detach_servo_retract_ts = 0;
  }

  // detach radial servo ---------------------------
  if (ts >= detach_servo_radial_ts && detach_servo_radial_ts != 0) {
    servo_radial.detach();
          
    cap.begin(0x5A);
    delay(50);
    cap.setThresholds(6, 1); 
    detach_servo_radial_ts = 0;
  }


  // close solenoids---------------------------
  if (ts >= ts_sol_offset && ts_sol_offset != 0) {           // if time is after solenoid offset time
    for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each sol
      digitalWrite(pinSol[i_sol], LOW);

      if (i_sol == num_spouts - 1) {
        Serial.print(14); Serial.print(" "); Serial.println(ts); // print sol offset
        ts_sol_offset = 0;    // reset solenoid offset time to close if statement
      }
    }
  }

  // turn off ttls for external time stamps ------------------------
  // lick---
  if (ts >= ts_lickomter_ttl_off && ts_lickomter_ttl_off != 0) {
    for (uint8_t i_sol = 0; i_sol < num_spouts; i_sol++) { // for each sensor (change the maximum i if more touch sensors are added)
      digitalWrite(pinLickometer_ttl[i_sol], LOW); // write ttl low
    }
    ts_lickomter_ttl_off = 0;            // reset off time to close if statement
  }

  // solenoid---
  if (ts >= ts_sol_ttl_off && ts_sol_ttl_off != 0) {
    digitalWrite(pinSol_ttl, LOW); // write ttl low
    ts_sol_ttl_off = 0;            // reset off time to close if statement
  }


  // session initialization (runs once at start) -----------------------------------------------------------------
  if (first_loop) {
    Serial.print(1);   Serial.print(" "); Serial.println(ts);             // print start session
    Serial.print(127); Serial.print(" "); Serial.println(trial_count);            // print session duration
    Serial.print(104); Serial.print(" "); Serial.println(session_retract);             // print session_retract

    Serial.print(128); Serial.print(" "); Serial.println(min_iti);                   // print min_delay
    Serial.print(129); Serial.print(" "); Serial.println(max_iti);                   // print max_delay

    Serial.print(115); Serial.print(" "); Serial.println(access_time);                 // print access_time
    Serial.print(117); Serial.print(" "); Serial.println(num_sol);                     // print num_sol
    Serial.print(118); Serial.print(" "); Serial.println(inter_sol_time);              // print inter_sol_time



    // retract spout
    if (session_retract) {
      servo_retract.attach(pinServo_retract);
      Serial.print(15); Serial.print(" "); Serial.println(ts);
      servo_retract.write(servo_retract_retracted_deg);

    }

    // allow time for servo travel and detach
    delay(500);
    servo_retract.detach();

    // set time for first delivery
    ts_access_start = ts + random(min_iti, max_iti);           // calculate first trial start

    first_loop = 0;

  }

  // licking-----------------------------------------------------------------------------------------------------
  // check state of sensor to see if it is currently touched
  currtouched = cap.touched();

  // check to see if touch onset occured
  //for (uint8_t i = 1; i <= num_spouts; i++) { // for each sensor (change the maximum i if more touch sensors are added)
  for (uint8_t i = current_spout + 1; i <= current_spout + 1; i++) { // for each sensor (change the maximum i if more touch sensors are added)
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) { // if touched now but not previously
      if (lick_gate) {
        lick = i;                                               // flag lick
      }
    }
  }

  // save current state for comparision with next state
  lasttouched = currtouched;

  // programed consequences to licking
  if (lick > 0) { // if lick has occured
    digitalWrite(pinLickometer_ttl[lick - 1], HIGH);                          // turn on ttl for external ts
    Serial.print(30 + lick); Serial.print(" "); Serial.println(ts); // print lick onset
    ts_lickomter_ttl_off = ts + ttl_duration;                       // set ttl offset time

    if (access_type == 0) {
      digitalWrite(pinSol[lick - 1], HIGH);                          // open solenoid for touched spout
      Serial.print(40 + lick); Serial.print(" "); Serial.println(ts); // print sol opening onset
      ts_sol_offset = ts + sol_duration[lick - 1];                            // set solenoid close time
    }

    lick = 0; // reset lick flag to close if statement
  }

  // schedule of magazine training --------------------------------------------------------------
  // start access period------------
  if (ts >= ts_access_start && ts_access_start != 0) {
    digitalWrite(pinTrial_ttl, HIGH);
    if (session_retract) {
      fun_servo_retract_extended(); // extend spout

    }
    ts_servo_retract_retracted = ts + access_time;      // set time for retract spout

    if (access_type == 1) {
      ts_sol_onset = ts;                                  // set solenoid onset time to current time
    }

    lick_gate = 1;
    tone(tone_freq, access_time);
    ts_access_start = 0;
  }

  // open solenoid num_sol times----
  if (ts >= ts_sol_onset && ts_sol_onset != 0 &&  count_sol < num_sol) {
    digitalWrite(current_sol, HIGH);         // open solenoid
    ts_sol_offset = ts + sol_duration[current_spout];  // set solenoid close time

    Serial.print(41 + current_spout); Serial.print(" "); Serial.println(ts); // print sol opening onset

    digitalWrite(pinSol_ttl, HIGH);      // turn on ttl for solenoid onset
    ts_sol_ttl_off = ts + ttl_duration; // set ttl offset time

    ts_sol_onset = ts + inter_sol_time; // set next solenoid onset time
    count_sol++;                                       // increase counter for solenoid
  }

  // end access period--------------
  if (ts >= ts_servo_retract_retracted && ts_servo_retract_retracted != 0) {
    digitalWrite(pinTrial_ttl, LOW);
    
    if (session_retract) {
      fun_servo_retract_retracted(); // retract spout
      
    }
    ts_servo_retract_retracted = 0;                     // reset retract time to close if statement

    // reset various variables and counters
    ts_sol_onset = 0;
    count_sol = 0;

    ts_access_start = ts + random(min_iti, max_iti);           // calculate first trial start

    count_trial++;
    current_spout = spout_block_set[count_trial]; // set current_spout
    current_sol = pinSol[current_spout];          // set current_sol

    lick_gate = 0;
    
    // spout switch----------------------------------
    initiate_spout_seq = 1;
    toggle_spout_update = 1;
  }

  // session termination ---------------------------------------------------------------------------------------------------
  // end session after trials exceed trial_count
  if (count_trial >= trial_count && session_end_ts == 0) {
    session_end_ts = ts + min_iti - 1000; // set end of session to 1s shorter than minimum ITI
  }

  if (ts > session_end_ts && session_end_ts != 0) {
    fun_end_session();
    Serial.print(99); Serial.print(" "); Serial.println(ts);    // print end of session
  }
}

// _______________________________________________________________________________________________________________________________________________________________________________
/// functions & interupts_________________________________________________________________________________________________________________________________________________________
void spout_update() {
  // break
  if (toggle_spout_update && count_trial < trial_count) {
    if (initiate_spout_seq) { // if sequence has been initiated and not in access period
      fun_servo_break_engage();
      move_servo_retract_retracted_ts = ts + detach_servo_break_step + detach_buffer;


      current_spout = spout_block_set[count_trial];


      initiate_spout_seq = 0;
    }

    // retract
    if (ts >= move_servo_retract_retracted_ts && move_servo_retract_retracted_ts != 0) {
      fun_servo_retract_retracted();
      move_servo_radial_sweep_ts = ts + detach_servo_retract_step + detach_buffer;
      move_servo_retract_retracted_ts = 0;
    }

    // move radial position
    if(ts >= move_servo_radial_sweep_ts && move_servo_radial_sweep_ts != 0){
      servo_radial.attach(pinServo_radial);
      servo_radial.write(sweep_deg); // sweep to signal block transition
      move_servo_radial_ts = ts + 500;

      move_servo_radial_sweep_ts = 0;
    }
    
    if (ts >= move_servo_radial_ts && move_servo_radial_ts != 0) {
      servo_radial.write(servo_radial_degs[current_spout]);
      detach_servo_radial_ts = ts + detach_servo_radial_step + detach_buffer;

      Serial.print(130); Serial.print(" "); Serial.println(ts);   // print radial position moved
      Serial.print(127); Serial.print(" "); Serial.println(ts);
      Serial.print(127); Serial.print(" "); Serial.println(current_spout);

      move_servo_radial_ts = 0;

      toggle_spout_update = 0;
    }
  }

/*
  if(ts >= ts_print_cap_step){
    fun_print_cap();
    ts_print_cap_step = ts + print_cap_step;
  }
*/

}

// servos ----------------------------------------------------------------
void fun_servo_break_engage() { //-----------------------------------------
  servo_break.attach(pinServo_break);
  servo_break.write(servo_break_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);
  detach_servo_break_ts = ts + detach_servo_break_step;
}

void fun_servo_break_disengage() { //--------------------------------------
  servo_break.attach(pinServo_break);
  servo_break.write(servo_break_disengaged_deg);
  Serial.print(12); Serial.print(" "); Serial.println(ts);
  detach_servo_break_ts = ts + detach_servo_break_step;
}

void fun_servo_retract_extended() { //-------------------------------------
  servo_retract.attach(pinServo_retract);
  servo_retract.write(servo_retract_extended_degs[current_spout]);
  Serial.print(13); Serial.print(" "); Serial.println(ts);
  detach_servo_retract_ts = ts + detach_servo_retract_step;
}

void fun_servo_retract_retracted() { //------------------------------------
  servo_retract.attach(pinServo_retract);
  servo_retract.write(servo_retract_retracted_deg);
  Serial.print(15); Serial.print(" "); Serial.println(ts);
  detach_servo_retract_ts = ts + detach_servo_retract_step;
}

/// end session -------------------------------------------------------------------------------------------
void fun_end_session() {
  servo_break.attach(pinServo_break);
  servo_break.write(servo_break_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);

  servo_retract.attach(pinServo_retract);
  servo_retract.write(servo_retract_retracted_deg);
  Serial.print(15); Serial.print(" "); Serial.println(ts);

  delay(500);
  servo_break.detach();
  servo_retract.detach();

  Serial.print(0); Serial.print(" "); Serial.println(ts);    // print end of session
  while (1) {}                             //  Stops executing the program
}


void fun_print_cap(){
    // debugging info, what
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();
}
