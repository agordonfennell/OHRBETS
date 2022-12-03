/*
-General notes--------------------------
* This program is an operant task with multiple schedule and reinforcement options
* The ratio or reinforcement can be either fixed or progressive
* The reinforcer can be either liquid reward (1 or more solenoid openings) or laser stimulation (on or off)
* This program includes an optional break control to limit the subjects ability to make operant responses
* This program includes an optional retractable spout to limit the subjects ability to make consumitory responses
* Solenoid onset starts with spout extension
* access period and solenoid  opening are independent
  * check to ensure that the access period is longer than (sol_duration + inter_sol_time) x num_sol
* All events (except dynamic parameters) that occur during the arduino program are transmitted via serial with an id and time stamp in ms (2 columns)
  * If you adapt this program and add new events, make sure to follow the 2 column approach to avoid conflicts with
    existing programs
  * Dynamic parameters are represented by 2 rows, the first row is the time stamp for the parameter, and the second is the current parameter
    the column will always contain the id for the dynamic parameter
    - it is critical that the order is maintaied
-Hardware------------------------------
* For a list of hardware see url ....
* This code was writen specifically for the following hardware
  * Micro servo: Tower Pro SG92R
    * other servos have not been tested. If you are using another servo make sure that they don't move when detatched 
  * Capacitive touch: Adafruit 12-Key Capacitive Touch Sensor Breakout MPR121 
  * Solenoid: Parker Series 3 003-0257-900
    * The solenoid duration may need to be calibrated for each solenoid based on multiple factors (height of 
      resivoir, tubing length, etc.)
* program is using sensor 1 for the Adafuit Capactitive touch sensor
  * additional sensors can be added

-Dependencies---------------------------
This program uses multiple dependencies
* Servo.h: library used to control micro servos used for break / retractable spout 
* Wire.h: library used for capacitive touch sensor
* Adafuit_MPR121.h: library used for capacitive touch sensor
  - NOTE: this library must be downloaded through the arduino library ("Tools">"Manage Libraries..."> search for MPR1221)
  - ***some versions of arduino do not include additional dependencies that Adafuit_MPR121.h relies on*** 
  - if you run into problems, uninstall arduino and install it thorugh the microsoft store (this has fixed this exact problem
    multiple times)

-Running the program--------------------------
* Set your parametrs within the program and then upload to your arduino board (make sure the correct COM port / 
  Board /Processor is selected under Tools)
  * break will engage and spout will extend (check to ensure this occured)
* The arduino program will wait for a serial command from python before it begins
* Initiate the arduino program by runing a computer program that sends a serial command to the arduino
  * make sure that correct comport and baudrate are selected
* All events that occur during the arduino program are transmitted via serial with an id and time stamp in ms 
* At the end of the session, the break will be engaged and spout will be retracted
* In between subjects, restart the arduino by resetting USB connection or reuploading the program. 

*** critical note *****************************
* currently, all parameters must be set in this arduino program
* Once the prgoram has been tested and polished, the ability to send parameters via serial will be added
* Save your parameters somewhere so they can be used to interpret your data

-Programing notes------------------------------
* Interupts are necessary to record rotation pulses from the rotary encoder (digitalRead/Write are to slow and miss rotation)
* Variables within interupts should be set as volatile, as their values will be changing rapidly and should always be in memory.
* You must use a fast serial Baud rate to avoid data lost from the rotary encoder (115200 is sufficient)

-Version notes---------------------------------
v01 adapted from fr_rotary_encoder_v13

v02
- added ability to run without break using variable session_break (0: no break, 1: break)
  - always breaks before session and at session end
  - with no break, rotation during the access period is recorded but does not count twoards the next reward
- added ability to run without spout retracting using variable session_retract(0: no retraction, 1 retraction)
  - always retracts at end of session
- added ability to run without serial input using variable session_serial_initiate(0: start without serial, 1: wait for serial input)
- added session contingency with variable session_contingency_current(0: right = active, 1: left = active)
- added in laser on/off for reinforcer
- added option for breaked timeout period beyond access/laser period
- added in progressive ratio
- added in print out at start of session with all parameters
- added in ability to change contingency during session 
- added in ability to remove reinforcer during session
  - rotation during non reinforced periods are recorded as to rotations 

v03
- changed pr formula
  - pr_function now sets the multiplier
      -  1: linear
      - >1: logorithmic 
  - fixed problem with break not disengaging at start of session
  - added progressive ratio timeout timer. Each reinforcer resets the timer to current time + pr_timeout. If no reinforcer is reached
    before the timer is reached, then the session is ended
  - added serial print for session end type (98: pr time out, 99: session duration time out)

v04
- added setback option to prevent subtraction in opposite direction (session_setback; 0: no setback, 1: setback to zero, 2: setback to negative)
  
-Todo------------------------------------------
- add in additional optional delay from tone to spout
  - remove subtract opposite direction

- include opto manipulations during different stages of the task 
  - open-loop with a set stim_tm_step
  - closed-loop during access period
  - closed-loop after access period.  
  
- clean up active / inactive / right / left code? 
  - currently downsampled rotation is recorded as active / inactive, but it may benifit us to record left / right instead
  - of two minds, doesn't really matter so long as we are tracking the contingency and we are careful to analyze correctly

long term todo...
- add parameters via serial?
- setup website with scripts / hardware list

- write out description of rotary encoder code
- write up description of trial control timing code

-serial event ids------------------------------
---parameters---
- 100 session_duration
- 101 rotary_resolution
- 103 session_break
- 104 session_retract
- 105 session_serial_initiate
- 126 session_setback

- 107 session_reinforcer
- 108 laser_posneg
- 109 schedule
- 110 pr_step
- 111 pr_function
- 112 break_sol_delay
- 113 min_delay
- 114 max_delay
- 115 access_time
- 116 sol_duration
- 117 num_sol
- 118 inter_sol_time
- 119 laser_duration
- 120 duration_additional_to
- 121 tone_freq
- 122 tone_duration

- 124 tm_switch_contingency_step
- 125 tm_switch_reinforcer_availability_step

---dynamic parameters--- 
[[2 line method: first row shows ts in 2nd column second row shows current value in 2nd colum]]
 
- 102 current_ratio
- 106 session_contingency_current 
- 123 session_reinforcer_availability

---events---
- 1   start_session
- 14  sol_offset
- 16  laser_onset
- 17  laser_offset
- 81  active_rotation
- 82  active_rotation_critiera
- 71  inactive_rotation
- 72  inactive_rotation_critiera
- 31  lick
- 11  break_engaged
- 12  break_disengaged
- 13  spout_extended
- 15  spout_retracted
- 83  active_rotation_to
- 73  inactive_rotation_to 
- 0   end_session
- 99  session duration time out
- 98  progressive ratio time out 
*/

// dependencies
  #include <Wire.h>
  #include "Adafruit_MPR121.h"
  #include <Servo.h>

// session parameters *************************************************************************************************************
 // general parameters -------
  static unsigned long session_duration = 1800000; // session duration (ms) (note: avoid using formula here, sometimes it leads to errors)
  static boolean session_break = 1;            // 0: no break during access period, 1: break engaged during access period
  static boolean inactive_break = 1;
  static boolean session_retract = 1;          // 0: no retractable spout, 1: retractable spout
  static boolean session_serial_initiate = 1;  // 0: starts without serial input, 1: waits for serial input before starting
  boolean session_contingency_current = 0;     // 0: right = active, 1: left = active
  boolean session_reinforcer_availability = 0; // 0: availible, 1: not availible
  static boolean session_reinforcer = 0;       // 0: liquid, 1: laser
  static boolean session_setback = 1;          // 1: no setback, 1: setback to zero, 2: setback to negative
  static boolean laser_posneg = 0;             // 0: pos reinforcement, 1: neg reinforcement

// switch parameters ---------
  static long tm_switch_contingency_step = -1;             // switch contingency every x ms (-1: no switching)
  static long tm_switch_reinforcer_availability_step = -1; // switch between reinforcer being available or witheld every x ms (-1: no switching)

 // rotation & schedule ------
  static int rotary_resoltion = 32; // number of rotation pulses per down sampled rotation pulse(raw PPR = 1024, rotary resolution of 16 -> 64 down sampled pulses per rotation (1024 / 16 = 64))
  static boolean schedule = 0;      // 0: fixed ratio, 1: progressive ratio
  int current_ratio = 8;           // number of down sampled rotation pulses per reward (absolute rotation will depend on both current_ratio and rotary_resolution, e.g. rotary resolution of 16(64 down sampled pulses per rotaiton), and a current_ratio of 32 means 1/2 rotation per reward)
  
  
  static int pr_step = 8;                   // step in number of down sampled rotation pusles per reward
  static double pr_function = 1.25;         // 1: linear, >1: logorithmic (1.25 aproximates Richardson and Roberts (1996)  [5e(R*0.2)] - 5 )
  static unsigned long pr_timeout = 600000; // amount of time wihtout a response before session ends (ms)

 // pre access delay ---------
  static unsigned long break_sol_delay = 500;   // minimum delay from break onset to access period start (allows for break to fully engage)
  static unsigned long min_delay = 0;           // minimum delay after break engages before access period start
  static unsigned long max_delay = 0;           // maximum delay after break engages before access period start
  
 // access & solenoid --------
  static int access_time = 3000;          // total time before spout is retracted
  
  static byte sol_duration = 40;          // duraiton solenoid is open during delivery
  static byte num_sol = 5;                // number of solenoid opening access period
  static int inter_sol_time = 140;        // interval between each solenoid opening (measured from end of previous until start of next)

 // opto --------------------
  static int laser_duration = 2000;

 // additional time out -----
  static int duration_additional_to = 0; // time out period (ms) that extends beyond access/opto period
 
 // tone cue -----------------
  static unsigned long tone_freq = 5000;  // tone played during access period / laser stim period (Hz)
  static int tone_duration = access_time; // tone duration (starts with begining of access time)
  

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinLickometer = 6;   
  static byte pinRotaryEncoderA = 3; 
  static byte pinRotaryEncoderB = 2; 
  
 // outputs ---------------------------
  static byte pinServo_break = 38;
  static byte pinServo_retract = 36;
  static byte pinSol = 7; 
  static byte pinLaser = 47;
  static byte pinSpeaker = 12;
  static byte pinImageStart = 41;     
  static byte pinImageStop = 42;  

  // ttls for external time stamps
  static byte pinSol_ttl = 52;
  static byte pinLickometer_ttl = 46;
  static byte pinRotation_active_ttl = 50;
  static byte pinRotation_inactive_ttl = 48;

//capicitance sensor variables
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;
  // note: code is setup to use sensor position 1

// servo break variables / parameters ------------------------------------------------------
  Servo servo_break;  // create servo object to control a servo
  static byte servo_break_engaged_deg = 15;
  static byte servo_break_disengaged_deg = 0;
  unsigned long detach_servo_break_ts = 0;
  static int detach_servo_break_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_extended_deg = 180;
  static byte servo_retract_retracted_deg = 120;
  unsigned long detach_servo_retract_ts = 0;
  static int detach_servo_retract_step = 100; // time in ms to allow for the servo to travel
  unsigned long ts_servo_retract_retracted;


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
  boolean rotation_gate = 1; // used to gate rotation relative to break engagement

// counters ---------------------------------------------------------------------------------
 // variables---
  volatile byte count_sol;                 // counter for number of solenoid openings per access period
  unsigned long pr_step_current = pr_step; // current step increase (remains stable if using linear pr_function)

// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile unsigned long ts; 
  unsigned long ts_start;
  unsigned long ts_access_start; 
  unsigned long ts_sol_offset;
  unsigned long ts_sol_onset;
  unsigned long ts_sol_ttl_off;
  unsigned long ts_laser_offset;
  unsigned long ts_lickomter_ttl_off; 
  unsigned long ts_rotation_active_ttl_off = 0;
  unsigned long ts_rotation_inactive_ttl_off = 0;
  unsigned long ts_to_end;
  volatile unsigned long ts_pr_timeout;

  static unsigned long tm_switch_contingency;         
  static unsigned long tm_switch_reinforcer_availability; 

  volatile unsigned long break_spout_delay; // random delay length between break and access period
  
  unsigned long session_end_ts;

 // parameters---
  static int ttl_duration = 5; // duration of tttl pulses for external time stamps (ms)
  

// _______________________________________________________________________________________________________________________________________________________________________________
/// setup ________________________________________________________________________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); 

 // engage servo break prior to session start
  servo_break.attach(pinServo_break); 
  Serial.print(11); Serial.print(" "); Serial.println(ts);      
  servo_break.write(servo_break_engaged_deg);

 // fully extend spout prior to session start if using liquid reinforcer
  if(session_reinforcer == 0){
    servo_retract.attach(pinServo_retract); 
    Serial.print(13); Serial.print(" "); Serial.println(ts);      
    servo_retract.write(servo_retract_extended_deg);
  }

 // retract spout piror to session if using laser reinforcer
  if(session_reinforcer == 1){
    servo_retract.attach(pinServo_retract); 
    Serial.print(15); Serial.print(" "); Serial.println(ts);      
    servo_retract.write(servo_retract_retracted_deg);
  }

 // delay with enough time for servos to move then detach
  delay(1500);
  servo_break.detach();
  servo_retract.detach();

 // define inputs --------------------------------
  pinMode(pinRotaryEncoderA,INPUT_PULLUP);
  pinMode(pinRotaryEncoderB,INPUT_PULLUP);
  pinMode(pinLickometer, INPUT);
  
 // define outputs
  pinMode(pinSol, OUTPUT);
  pinMode(pinLaser, OUTPUT);
  pinMode(pinSpeaker, OUTPUT);
  pinMode(pinImageStart, OUTPUT);
  pinMode(pinImageStop, OUTPUT);

  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinRotation_active_ttl, OUTPUT);
  pinMode(pinRotation_inactive_ttl, OUTPUT);
  pinMode(pinLickometer_ttl, OUTPUT);

  // setup capative touch sesnsor ---------------
  if (!cap.begin(0x5A)) {                   // if the sensor is not detected
    Serial.println("MPR121 not detected!"); // print warning (and crash python program)
    while (1);
  }

  cap.setThresholds(6,2);
  delay(50);
  Serial.print(999);   Serial.print(" "); Serial.println(cap.filteredData(1));

 // rotary encoder ------------------------------
 // setup interupts for the rotary encoder 
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderB),fpinRotaryEncoderB,RISING); // interrupt on pinRotaryEncoderA
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderA),fpinRotaryEncoderA,RISING); // interrupt on pinRotaryEncoderB
  
 // save starting state of rotary encoder
  maskA = digitalPinToBitMask(pinRotaryEncoderA);
  maskB = digitalPinToBitMask(pinRotaryEncoderB);
  maskAB = maskA | maskB;
  port = portInputRegister(digitalPinToPort(pinRotaryEncoderA));


 // wait for serial command before initating session---------------------------------------------------------------------
  if(session_serial_initiate){
    while (Serial.available() <= 0) {} // wait for serial input to start session
  }

 // save start time and send ttl to initate scope
  ts_start=millis();  
  digitalWrite(pinImageStart, HIGH);
  delay(100);
  digitalWrite(pinImageStart, LOW);
}


/// void loop ____________________________________________________________________________________________________________________________________________________________________
// _______________________________________________________________________________________________________________________________________________________________________________
void loop() {
// background functions (run constantly independent of task events)--------------------------------------------
 // generate timestamp ---------------------------
  ts=millis()-ts_start;

 // detach break servo ---------------------------
  if(ts >= detach_servo_break_ts && detach_servo_break_ts!=0){
    servo_break.detach();
    detach_servo_break_ts = 0;
  }

 // detach spout servo ---------------------------
  if(ts >= detach_servo_retract_ts && detach_servo_retract_ts!=0){
    servo_retract.detach();
    detach_servo_retract_ts = 0;
  }

 // close solenoids---------------------------
  if(ts>=ts_sol_offset && ts_sol_offset!=0){                 // if time is after solenoid offset time
    digitalWrite(pinSol, LOW);                               // close solenoid
    Serial.print(14); Serial.print(" "); Serial.println(ts); // print sol offset
    ts_sol_offset = 0;    // reset solenoid offset time to close if statement
    }

 // turn off ttls for external time stamps ------------------------
 // lick---
  if(ts>=ts_lickomter_ttl_off && ts_lickomter_ttl_off!=0){
    digitalWrite(pinLickometer_ttl,LOW); // write ttl low
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
    Serial.print(1);   Serial.print(" "); Serial.println(ts);             // print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);            // print session duration
    Serial.print(101); Serial.print(" "); Serial.println(rotary_resoltion);            // print rotary_resoltion
    Serial.print(103); Serial.print(" "); Serial.println(session_break);               // print session_break
    Serial.print(104); Serial.print(" "); Serial.println(session_retract);             // print session_retract
    Serial.print(105); Serial.print(" "); Serial.println(session_serial_initiate);     // print session_serial_initiate
    Serial.print(107); Serial.print(" "); Serial.println(session_reinforcer);          // print session_reinforcer
    Serial.print(108); Serial.print(" "); Serial.println(laser_posneg);                // print laser_posneg
    Serial.print(109); Serial.print(" "); Serial.println(schedule);                    // print schedule
    Serial.print(110); Serial.print(" "); Serial.println(pr_step);                     // print pr_step
    Serial.print(111); Serial.print(" "); Serial.println(pr_function);                 // print pr_function
    Serial.print(112); Serial.print(" "); Serial.println(break_sol_delay);             // print break_sol_delay
    Serial.print(113); Serial.print(" "); Serial.println(min_delay);                   // print min_delay
    Serial.print(114); Serial.print(" "); Serial.println(max_delay);                   // print max_delay
    Serial.print(115); Serial.print(" "); Serial.println(access_time);                 // print access_time
    Serial.print(116); Serial.print(" "); Serial.println(sol_duration);                // print sol_duration
    Serial.print(117); Serial.print(" "); Serial.println(num_sol);                     // print num_sol
    Serial.print(118); Serial.print(" "); Serial.println(inter_sol_time);              // print inter_sol_time
    Serial.print(119); Serial.print(" "); Serial.println(laser_duration);              // print laser_duration
    Serial.print(120); Serial.print(" "); Serial.println(duration_additional_to);      // print duration_
    Serial.print(121); Serial.print(" "); Serial.println(tone_freq);                   // print tone_fr
    Serial.print(122); Serial.print(" "); Serial.println(tone_duration);               // print tone_duration
    Serial.print(124); Serial.print(" "); Serial.println(tm_switch_contingency_step);               // print tm_switch_contingency_step
    Serial.print(125); Serial.print(" "); Serial.println(tm_switch_reinforcer_availability_step);   // print tm_switch_reinforcer_availability_step
    Serial.print(126); Serial.print(" "); Serial.println(session_setback);             // print sessio
    
   // disengage break
    servo_break.attach(pinServo_break); 
    Serial.print(12); Serial.print(" "); Serial.println(ts);      
    servo_break.write(servo_break_disengaged_deg);

   // retract spout
    if(session_retract){ 
      servo_retract.attach(pinServo_retract); 
      Serial.print(15); Serial.print(" "); Serial.println(ts);      
      servo_retract.write(servo_retract_retracted_deg);
    }
    
   // allow time for servo travel and detach
    delay(500); 
    servo_break.detach();
    servo_retract.detach();

    if(session_reinforcer == 1 && laser_posneg == 1){ // if laser reinforcer and negative reinforcement
      digitalWrite(pinLaser, HIGH);
      Serial.print(16); Serial.print(" "); Serial.println(ts);    // print laser onset
      
      }
    
    session_end_ts = ts + session_duration; // set session end time 
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
  for (uint8_t i=1; i<2; i++) { // for each sensor (change the maximum i if more touch sensors are added)
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) { // if touched now but not previously
      lick=1;                                                 // flag lick
    }
  }

 // save current state for comparision with next state
  lasttouched = currtouched; 

 // programed consequences to licking
  if (lick==1){ // if lick has occured
      Serial.print(31); Serial.print(" "); Serial.println(ts); // print lick onset
      digitalWrite(pinLickometer_ttl,HIGH);                    // turn on ttl for external ts
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
        
        Serial.print(83); Serial.print(" "); Serial.println(ts); // print active rotation reaching criteria during breaked or non reinforced periods
        
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
      
      Serial.print(73); Serial.print(" "); Serial.println(ts); // print inactive rotation during breaked or non reinforced periods
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
        if(session_break){fun_servo_break_engage();}                // engage break
        rotation_gate = 0;                                          // close rotation gate
        
        break_spout_delay = random(min_delay, max_delay);           // calculate random delay
        ts_access_start = ts + break_sol_delay + break_spout_delay; // set access start time 

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

      
      
     // start access period------------
      if(ts >= ts_access_start && ts_access_start != 0){
        if(session_retract){fun_servo_retract_extended();}  // extend spout
        ts_servo_retract_retracted = ts + access_time;      // set time for retract spout
        ts_to_end = ts_servo_retract_retracted + duration_additional_to; // set end of to time
        ts_sol_onset = ts;                                  // set solenoid onset time to current time
        ts_access_start = 0;
        tone(pinSpeaker, tone_freq, tone_duration); 
      }

     // open solenoid num_sol times----
      if(ts >= ts_sol_onset && ts_sol_onset != 0 &&  count_sol < num_sol){
        digitalWrite(pinSol,HIGH);          // open solenoid
        ts_sol_offset = ts + sol_duration;  // set solenoid close time
        
        Serial.print(18); Serial.print(" "); Serial.println(ts); // print sol onset
        
        digitalWrite(pinSol_ttl,HIGH);      // turn on ttl for solenoid onset 
        ts_sol_ttl_off = ts + ttl_duration; // set ttl offset time

        ts_sol_onset = ts + inter_sol_time + sol_duration; // set next solenoid onset time
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

        if(session_break){fun_servo_break_disengage();}     // disengage break 
        rotation_gate = 1; // open rotation gate
        ts_to_end = 0;     // reset end of to time to close if statement
      }
      
    }
  }
  
 // laser reinforcer ----------------------------------------------------------------
  if(session_reinforcer == 1){ // if using laser as reinforcer and reinforcer is available
  if (rotation_active >= current_ratio) { // once down sampled rotation count reaches fixed ratio
     // initialize access period-----
      if(ts_access_start == 0 && ts_to_end == 0){ 
        if(session_break){fun_servo_break_engage();}                // engage break
        rotation_gate = 0;                                          // close rotation gate
        Serial.print(82); Serial.print(" "); Serial.println(ts);    // print reached criteria
        break_spout_delay = random(min_delay, max_delay);           // calculate random delay
        ts_access_start = ts + break_sol_delay + break_spout_delay; // set access start time
        
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

     // start breaked period and change laser state---
      if(ts >= ts_access_start && ts_access_start != 0){
        ts_access_start = 0;

        if(laser_posneg == 0){ // if positive reinforcement ----
          digitalWrite(pinLaser, HIGH);                               // turn on laser
          Serial.print(16); Serial.print(" "); Serial.println(ts);    // print laser onset
        }

        if(laser_posneg == 1){ // if negative reinforcement ----
          digitalWrite(pinLaser, LOW);                                // turn off laser
          Serial.print(17); Serial.print(" "); Serial.println(ts);    // print laser onset
        }

        ts_laser_offset = ts + laser_duration;                                  // set laser onset
        ts_to_end = ts_laser_offset + duration_additional_to;                   // set end of to time
        tone(pinSpeaker, tone_freq, tone_duration); 
      }

     // change laser state & end breaked peroid---
      if(ts >= ts_laser_offset && ts_laser_offset != 0){
        ts_laser_offset = 0; 
        
         if(laser_posneg == 0){ // if positive reinforcement ----
          digitalWrite(pinLaser, LOW);                                // turn off laser
          Serial.print(17); Serial.print(" "); Serial.println(ts);    // print laser onset
        }

        if(laser_posneg == 1){ // if negative reinforcement ----
          digitalWrite(pinLaser, HIGH);                               // turn on laser
          Serial.print(16); Serial.print(" "); Serial.println(ts);    // print laser onset
        }
      }

     // end time out------------------
     if(ts >= ts_to_end && ts_to_end != 0){   
        rotation_active = 0;
        rotation_active_counter_trial = 0;
        rotation_inactive = 0;
        rotation_inactive_counter_trial = 0;

        if(session_break){fun_servo_break_disengage();}     // disengage break  
        rotation_gate = 1;
        ts_to_end = 0;
      }
    }
  }

 // Inactive criteria --------------------------------------------------------------------------
  if (rotation_inactive >= current_ratio) {
    //break inactive direction for as long as active direction but without tone or reward
      if(inactive_break){
        if(ts_to_end == 0){
          rotation_gate = 0;                       // close rotation gate
          Serial.print(72); Serial.print(" "); Serial.println(ts); // print stim/tone onset
          fun_servo_break_engage();                // engage break
          ts_to_end = ts + break_sol_delay + access_time + duration_additional_to;
        }

        if(ts > ts_to_end && ts_to_end != 0){
         // reset various variables and counters
                  
          rotation_active = 0;
          rotation_active_counter_trial = 0;
          rotation_inactive = 0;
          rotation_inactive_counter_trial = 0;
  
          fun_servo_break_disengage();     // disengage break 
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
void fun_servo_break_engage(){ //-----------------------------------------
  servo_break.attach(pinServo_break);  
  servo_break.write(servo_break_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);            
  detach_servo_break_ts = ts + detach_servo_break_step;
}

void fun_servo_break_disengage(){ //--------------------------------------
  servo_break.attach(pinServo_break);  
  servo_break.write(servo_break_disengaged_deg);
  Serial.print(12); Serial.print(" "); Serial.println(ts);            
  detach_servo_break_ts = ts + detach_servo_break_step;
}

void fun_servo_retract_extended(){ //-------------------------------------
  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_extended_deg);
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
  digitalWrite(pinImageStop, HIGH);
  servo_break.attach(pinServo_break);  
  servo_break.write(servo_break_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);     

  servo_retract.attach(pinServo_retract);  
  servo_retract.write(servo_retract_retracted_deg);
  Serial.print(15); Serial.print(" "); Serial.println(ts); 
  
  delay(500);
  
  digitalWrite(pinImageStop, LOW); 
  servo_break.detach();  
  servo_retract.detach();  
  
  Serial.print(0); Serial.print(" "); Serial.println(ts);    // print end of session                 
  while(1){}                               //  Stops executing the program
}

  
 
