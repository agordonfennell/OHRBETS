/*
-General notes--------------------------
* This program is a simple sipper training task, where each lick triggers a solenoid opdning
* Breaks are engaged and sipper is extended at start of session
* sipper is retracted at end of session

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

-Version notes---------------------------------
v01 adapted from headfixed_rotary_operant_ratio_v05
  
-Todo------------------------------------------
- include imaging  frame input / serial

-serial event ids------------------------------
---parameters---
- 100 session_duration
- 105 session_serial_initiate

- 116 sol_duration

---dynamic parameters--- 
[[2 line method: first row shows ts in 2nd column second row shows current value in 2nd colum]]
- none

---events---
- 1   start_session
- 31  lick
- 11  break_engaged
- 12  break_disengaged
- 13  spout_extended
- 15  spout_retracted
- 0   end_session
- 99  session duration time out
*/

// dependencies
  #include <Wire.h>
  #include "Adafruit_MPR121.h"
  #include <Servo.h>

// session parameters *************************************************************************************************************
 // general parameters -------
  static unsigned long session_duration = 600000; // session duration (ms) (note: avoid using formula here, sometimes it leads to errors)
  static boolean session_serial_initiate = 1;  // 0: starts without serial input, 1: waits for serial input before starting

 // access & solenoid --------
  static byte sol_duration = 20;          // duraiton solenoid is open during delivery

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static byte pinLickometer = 6;   
  
 // outputs ---------------------------
  static byte pinServo_break = 38;
  static byte pinServo_retract = 36;
  static byte pinSol = 7;  

  // ttls for external time stamps
  static byte pinSol_ttl = 52;
  static byte pinLickometer_ttl = 46;

//capicitance sensor variables
  Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;
  // note: code is setup to use sensor position 1

// servo break variables / parameters ------------------------------------------------------
  Servo servo_break;  // create servo object to control a servo
  static byte servo_break_engaged_deg = 25;
  static byte servo_break_disengaged_deg = 5;
  unsigned long detach_servo_break_ts = 0;
  static int detach_servo_break_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_extended_deg = 180;
  static byte servo_retract_retracted_deg = 120;
  unsigned long detach_servo_retract_ts = 0;
  static int detach_servo_retract_step = 100; // time in ms to allow for the servo to travel
  unsigned long ts_servo_retract_retracted;

  
// flags -------------------------------------------------------------------------------------
 // variables---
  boolean solOpen;
  boolean lick;

// time stamps & timers ---------------------------------------------------------------------
 // variables---
  volatile unsigned long ts; 
  unsigned long ts_start;
  unsigned long ts_sol_offset;
  unsigned long ts_sol_onset;
  unsigned long ts_sol_ttl_off;
  unsigned long ts_lickomter_ttl_off; 
  
  unsigned long session_end_ts;

 // parameters---
  static int ttl_duration = 2; // duration of tttl pulses for external time stamps (ms)
  

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
  servo_retract.attach(pinServo_retract); 
  Serial.print(13); Serial.print(" "); Serial.println(ts);      
  servo_retract.write(servo_retract_extended_deg);

 // delay with enough time for servos to move then detach
  delay(1500);
  servo_break.detach();
  servo_retract.detach();

 // define inputs --------------------------------
  pinMode(pinLickometer, INPUT);
  
 // define outputs
  pinMode(pinSol, OUTPUT);


  pinMode(pinSol_ttl, OUTPUT);
  pinMode(pinLickometer_ttl, OUTPUT);

  // setup capative touch sesnsor ---------------
  if (!cap.begin(0x5A)) {                   // if the sensor is not detected
    Serial.println("MPR121 not detected!"); // print warning (and crash python program)
    while (1);
  }

  cap.setThresholds(6,2);
  delay(50);
  Serial.print(999);   Serial.print(" "); Serial.println(cap.filteredData(1));


 // wait for serial command before initating session---------------------------------------------------------------------
  if(session_serial_initiate){
    while (Serial.available() <= 0) {} // wait for serial input to start session
  }

 // save start time and send ttl to initate scope
  ts_start=millis();  
  delay(100);
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
  
// session initialization (runs once at start) -----------------------------------------------------------------
  if(session_end_ts == 0){
    Serial.print(1);   Serial.print(" "); Serial.println(ts);             // print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);            // print session duration
    Serial.print(116); Serial.print(" "); Serial.println(sol_duration);                // print sol_duration


    session_end_ts = ts + session_duration;
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

      digitalWrite(pinSol,HIGH);          // open sol
      ts_sol_offset = ts + sol_duration;  // set solenoid close time
      Serial.print(18); Serial.print(" "); Serial.println(ts); // print sol onset
      
      digitalWrite(pinLickometer_ttl,HIGH);                    // turn on ttl for external ts
      ts_lickomter_ttl_off = ts + ttl_duration;                // set ttl offset time

      lick=0; // reset lick flag to close if statement
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
  while(1){}                               //  Stops executing the program
}

  
 
