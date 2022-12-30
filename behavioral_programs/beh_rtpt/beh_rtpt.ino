
/*
-General notes--------------------------
* This program is a real-time place testing program (rtpt)
* The wheel is divided into 2 zones based on the starting position of the wheel
* location in one zone sets external ttl to high, the other zone sets to low
* options for pairing tones with the two zones 
* options for switching contingency during session

-Dependencies---------------------------
This program uses multiple dependencies (see protocol: for instructions on installation)
* Servo.h: library used to control micro servos used for brake / retractable spout 

-Data structure-------------------------
* see general arduino program approach for a description of how events and parameters are recorded

-Running the program--------------------------
* follow protocol for behavior script

*/

// dependencies
  #include <Servo.h>

 // parameters 
  unsigned long session_duration = 1200000; // ms

  unsigned long tm_switch_pairing_step = -1; // switch pairing every x ms (-1: no switching)
  unsigned long ts_switch_pairing = 0;

  // zone & tone parameters*************
  int zone = 2;  // initial pairing szone
  
  static boolean session_tone = 1; // 0: no tone for session; 1: tone to indicate paired / unpaired
  unsigned long tone_freq_initially_unpaired = 8000;
  unsigned long tone_freq_initially_paired = 5000;
  //**************************************
  
  int ppr = 1024;
  int rotary_resolution = 16; // 1024PPR total; number of pulses per encoding
  int ppr_encoded = 64; // ppr / rotary_resolution
  
  boolean toggle_pairing = 0; // 
  int ttl_duration = 5; // duration of ttl time stamp (ms)

// arduino pins ----------------------------------------------------------------------------
 // inputs ----------------------------
  static int pinRotaryEncoderA = 3;
  static int pinRotaryEncoderB = 2;
  
 // outputs ---------------------------
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;
  static byte pinSpeaker = 12;
  
  static byte rotation_left_ttl = 8;
  static byte rotation_right_ttl = 9;
  
  int pinextTTL = 46;

// variables ------------------------- 
  boolean session_pairing_current;
  volatile int rotation_right_counter_trial = 0;  
  volatile int rotation_left_counter_trial = 0;
  
  volatile boolean rotation_right_flag = 0;
  volatile boolean rotation_left_flag = 0;
  
  volatile int rotation_position;
  volatile int rotation_position_prev = rotation_position;

  
  unsigned long ts; 
  unsigned long ts_start;
  unsigned long session_end_ts;
  
 // rotary encoder-------------------------------
  volatile int rotation_right = 0;
  volatile int rotation_right_prev = 0;
  unsigned long rotation_right_ttl_off = 0;
  
  volatile int rotation_left = 0;
  volatile int rotation_left_prev = 0;
  unsigned long rotation_left_ttl_off = 0;
  
  int reading;

  volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderA to signal that the encoder has arrived at a detent
  volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoderB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
  volatile byte reading02 = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
  uint8_t maskA;
  uint8_t maskB;
  uint8_t maskAB;
  volatile uint8_t *port;

// servo brake variables / parameters ------------------------------------------------------
  Servo servo_brake;  // create servo object to control a servo
  static byte servo_brake_engaged_deg = 20;
  static byte servo_brake_disengaged_deg = 0;
  unsigned long detach_servo_brake_ts = 0;
  static int detach_servo_brake_step = 100; // time in ms to allow for the servo to travel

// servo retractable spout variables / parameters ------------------------------------------
  Servo servo_retract;
  static byte servo_retract_retracted_deg = 120;

// servo radial varialbe / parameters-------------------------------------------------------
  Servo servo_radial;  
  static byte servo_radial_deg = 0;

// setup _________________________________________________________________________________________
void setup() {
  Serial.begin(115200); // changed from 9600

 // define inputs
  pinMode(pinRotaryEncoderA,INPUT_PULLUP);
  pinMode(pinRotaryEncoderB,INPUT_PULLUP);
  
 // define outputs
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT); 
  pinMode(pinextTTL, OUTPUT);
  pinMode(pinSpeaker, OUTPUT);
  
  pinMode(rotation_right_ttl, OUTPUT);
  pinMode(rotation_left_ttl, OUTPUT);

 // save starting state of rotary encoder
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderB),fpinRotaryEncoderB,RISING); // set an interrupt on pinRotaryEncoderA, looking for a rising edge signal and executing the "pinRotaryEncoderA" Interrupt Service Routine (below)
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoderA),fpinRotaryEncoderA,RISING); // set an interrupt on pinRotaryEncoderB, looking for a rising edge signal and executing the "pinRotaryEncoderB" Interrupt Service Routine (below)

  maskA = digitalPinToBitMask(pinRotaryEncoderA);
  maskB = digitalPinToBitMask(pinRotaryEncoderB);
  maskAB = maskA | maskB;
  port = portInputRegister(digitalPinToPort(pinRotaryEncoderA));

    // zone 1 paired
  if(zone == 1){
   session_pairing_current = 0;
   rotation_position = ppr_encoded/2; // start position is at 50% of end position 
  }

  if(zone == 2){
   session_pairing_current = 1;
   rotation_position = ppr_encoded/2 + 1; // start position is at 50% of end position 
  }

// engage servo brake prior to session start
  servo_brake.attach(pinServo_brake); 
  Serial.print(11); Serial.print(" "); Serial.println(ts);      
  servo_brake.write(servo_brake_engaged_deg);
  delay(250);
  servo_brake.detach();

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
  
 // wait for serial command before initating session---------------------------------------------------------------------
  while (Serial.available() <= 0) {} // wait for serial input to start session
  delay(100);

 // save start time
  ts_start=millis();  
}



// _________________________________________________________________________________________________________________________________
/// void loop ______________________________________________________________________________________________________________________
// _________________________________________________________________________________________________________________________________
void loop() {
  // generate timestamp 
  ts=millis()-ts_start;

 // detach brake servo ---------------------------
  if(ts >= detach_servo_brake_ts && detach_servo_brake_ts!=0){
    servo_brake.detach();
    detach_servo_brake_ts = 0;
  }
 
  // session initialization (runs once at start) -----------------------------------------------------------------
  if(session_end_ts == 0){

    Serial.print(1);   Serial.print(" "); Serial.println(ts);                             // print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);               // print end ts
    Serial.print(101); Serial.print(" "); Serial.println(rotary_resolution);              // print rotary_resolution
    Serial.print(141); Serial.print(" "); Serial.println(tm_switch_pairing_step);         // print tm_switch_pairing_step
    Serial.print(131); Serial.print(" "); Serial.println(tone_freq_initially_paired);     // print tone_freq_initially_paired
    Serial.print(132); Serial.print(" "); Serial.println(tone_freq_initially_unpaired);   // print tone_freq_initially_unpaired

    Serial.print(140); Serial.print(" "); Serial.println(ts);                        // print time for session_pairing_current (ts must precede value)
    Serial.print(140); Serial.print(" "); Serial.println(session_pairing_current);   // print initial pairing

    Serial.print(142); Serial.print(" "); Serial.println(rotation_position);  // print initial position
    
   if(session_tone){tone(pinSpeaker, tone_freq_initially_unpaired);} // start in unpaired side
    
   // disengage brake
    servo_brake.attach(pinServo_brake); 
    Serial.print(12); Serial.print(" "); Serial.println(ts);      
    servo_brake.write(servo_brake_disengaged_deg);
    detach_servo_brake_ts = ts + detach_servo_brake_step;
    
    session_end_ts = ts + session_duration;
  }
  
  // pairing switch ------------------------------------------------
  if(ts_switch_pairing == 0 && tm_switch_pairing_step != -1){
    ts_switch_pairing = ts + tm_switch_pairing_step;
  }

  if(ts > ts_switch_pairing && ts_switch_pairing != 0){
    session_pairing_current = !session_pairing_current;

    if(toggle_pairing == 0){ // if in initially unpaired side 
      if(session_pairing_current){
        side_current_paired();
      } else {
        side_current_unpaired();
      }
    }

    if(toggle_pairing == 1){ // if in initially paired side
      if(session_pairing_current){
        side_current_unpaired();
      } else {
        side_current_paired();
      }
    }

    Serial.print(140); Serial.print(" "); Serial.println(ts);                        // print time for session_pairing_current (ts must precede value)
    Serial.print(140); Serial.print(" "); Serial.println(session_pairing_current);   // print session_pairing_current
    
    ts_switch_pairing = ts + tm_switch_pairing_step;
  }


// rotary encoder ------------------------------------------------------
  // right -------------------------------- 
   if(rotation_right_flag){
    rotation_right_counter_trial++;
    rotation_right_flag = 0;
   }

   if(rotation_right_counter_trial >= rotary_resolution){
      rotation_right++;
      rotation_position++;
      
      digitalWrite(rotation_right_ttl,HIGH);
      rotation_right_ttl_off = ts + ttl_duration;
  
      Serial.print(81); Serial.print(" "); Serial.println(ts); // print right rotation reaching criteria
  
      rotation_right_counter_trial = 0;
   }
    
  // left ---------------------------------
   if(rotation_left_flag){
    rotation_left_counter_trial++;
    rotation_left_flag = 0;
   }
  
   if(rotation_left_counter_trial >= rotary_resolution){
      rotation_left++;
      rotation_position--;
      
      digitalWrite(rotation_left_ttl,HIGH);
      rotation_left_ttl_off = ts + ttl_duration;
      
      Serial.print(71); Serial.print(" "); Serial.println(ts); // print left rotation

      rotation_left_counter_trial = 0;
   }

 // turn on / off extTTL based on position ---------------------------------
  if(rotation_position != rotation_position_prev){
    if(rotation_position <= 0){
      rotation_position = ppr_encoded;
      rotation_position_prev = ppr_encoded;
    }
  
    if(rotation_position > ppr_encoded){
      rotation_position = 1;
      rotation_position_prev = ppr_encoded;
    }
  
    Serial.print(149); Serial.print(" "); Serial.println(rotation_position); // print rotation position (must use right/left rotation for ts)
  
    if(rotation_position <= ppr_encoded/2 && toggle_pairing == 1){
      if(session_tone){tone(pinSpeaker, tone_freq_initially_unpaired);}    
      Serial.print(28); Serial.print(" "); Serial.println(ts); // print entry to initially unpaired side
  
      if(session_pairing_current == 0){
        side_current_unpaired();
      } else {
        side_current_paired();
      }
  
      toggle_pairing = 0;
    } 
  
    if(rotation_position > ppr_encoded/2 && toggle_pairing == 0){
      if(session_tone){tone(pinSpeaker, tone_freq_initially_paired);} 
      Serial.print(27); Serial.print(" "); Serial.println(ts); // print entry to initially paired side
      
      if(session_pairing_current == 0){
        side_current_paired();
       } else {
        side_current_unpaired();
      } 
  
      toggle_pairing = 1;
    }
  }

  rotation_position_prev = rotation_position;
    
 // turn off ttl marker
  if(ts>=rotation_right_ttl_off && rotation_right_ttl_off!=0){
    digitalWrite(rotation_right_ttl,LOW);
    rotation_right_ttl_off = 0;
  }

  if(ts>=rotation_left_ttl_off && rotation_left_ttl_off!=0){
    digitalWrite(rotation_left_ttl,LOW);
    rotation_left_ttl_off = 0;
  }

 // session duration termination
  if(ts > session_end_ts && session_end_ts != 0){
    fun_end_session();
  }
}

// _________________________________________________________________________________________________________________________________
/// functions & interupts______________________________________________________________________________________________________________________
// _________________________________________________________________________________________________________________________________
void side_current_unpaired(){
    digitalWrite(pinextTTL, LOW);
    Serial.print(16); Serial.print(" "); Serial.println(ts); // print extTTL offset
}

void side_current_paired(){
    digitalWrite(pinextTTL, HIGH);
    Serial.print(17); Serial.print(" "); Serial.println(ts); // print extTTL onset
}

void fpinRotaryEncoderB(){
  noInterrupts();
  reading02 = *port & maskAB;
  if((reading02 == maskAB) && aFlag) {

  rotation_left_flag = 1;  
  
  bFlag = 0;
  aFlag = 0;
  }
  else if (reading02 == maskB) bFlag = 1;
  interrupts();
}

void fpinRotaryEncoderA(){
  noInterrupts();
  reading02 = *port & maskAB;
  if (reading02 == maskAB && bFlag) {
  rotation_right_flag = 1;
  
  bFlag = 0;
  aFlag = 0;
  }
  else if (reading02 == maskA) aFlag = 1;
  interrupts();
}


/// end session -------------------------------------------------------------------------------------------
void fun_end_session() {
  servo_brake.attach(pinServo_brake);  
  servo_brake.write(servo_brake_engaged_deg);  
  delay(250);
  servo_brake.detach();    

  if(session_tone){noTone(pinSpeaker);}
  
  while(1){}                                                 // Stops executing the program
}
  
 
