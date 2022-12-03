
/*
notes:
- hab / preferenes test: - start position = middle
- remaining days:
  - counter ballance zones 
*    - requires setting the start position and the pairing 

*/
 #include <Wire.h>
 #include "Adafruit_MPR121.h"
 #include <Servo.h>

//capicitance sensor
 Adafruit_MPR121 cap = Adafruit_MPR121();
  uint16_t lasttouched = 0;
  uint16_t currtouched = 0;

 // parameters to adjust //////////////////////
  unsigned long session_duration = 900000;//

  unsigned long tm_switch_pairing_step = -1;// 30000; // session_duration/2
  unsigned long tm_switch_pairing_ts = 0;
  boolean session_pairing_current = 0;
  
  
  int ppr = 1024;
  int rotary_resolution = 16; // 1024PPR total; number of pulses per encoding
  int ppr_encoded = 64; // ppr / rotary_resolution
  
  unsigned long tone_freq_initially_unpaired = 0;// 5000; 
  unsigned long tone_freq_initially_paired = 0; // 8000; 
  ////////
  
  int ttl_duration = 2;

 // setup pins
  static byte pinServo_break = 38;
  int pinLaser = 25;
  int pinLaser_ttl = 5; //52
  unsigned long pinLaser_ttl_off = 0;
  
  int pinSpeaker = 46;
  int pinLickometer = 6;   
  int pinLickometer_ttl = 4; //46
  unsigned long pinLickometer_ttl_off = 0;
  
  int pinImageStart = 13; // 41;     
  int pinImageStop = 1; // 42;  
  int pinFrame = 0; // 40;

  static int pinRotaryEncoder01 = 3; // A
  static int pinRotaryEncoder02 = 2; // B
  volatile int rotation_right_counter_trial = 0;  
  volatile int rotation_left_counter_trial = 0;
  
  volatile boolean rotation_right_flag = 0;
  volatile boolean rotation_left_flag = 0;

  
 // 
  boolean pinFrame_read = 0;
  boolean pinFrame_read_previous = 0;

//  volatile int rotation_position = ppr_encoded/2/2; // start position is at 25% of end position
  volatile int rotation_position = ppr_encoded/2; // start position is at 50% of end position  
  volatile int rotation_position_prev = rotation_position;

  boolean toggle_initially_paired = 0;
  
  volatile int rotation_right = 0;
  volatile int rotation_right_prev = 0;
  int rotation_right_ttl = 9; //50;
  int rotation_right_to_ttl = 9; // change this
  unsigned long rotation_right_ttl_off = 0;
  unsigned long rotation_right_to_ttl_off = 0;
  
  volatile int rotation_left = 0;
  volatile int rotation_left_prev = 0;
  int rotation_left_ttl = 8; //48;
  int rotation_left_to_ttl = 8; // change this
  unsigned long rotation_left_ttl_off = 0;
  unsigned long rotation_left_to_ttl_off = 0;
  
  boolean solOpen = 0;
  boolean frame = 0;
  boolean lick;
  boolean pinLickometer_read = 0;
  boolean pinLickometer_read_previous = 0;
  int rotation_right_count = 0;

  unsigned long ts; 
  unsigned long ts_start;
  unsigned long ts_sol_offset = 0;
  unsigned long session_end_ts;
  
 // arduino
  int reading;

  volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoder01 to signal that the encoder has arrived at a detent
  volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinRotaryEncoder02 to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
  volatile byte reading02 = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
  
  // new stuff here...
  uint8_t maskA;
  uint8_t maskB;
  uint8_t maskAB;
  volatile uint8_t *port;


// servo break variables / parameters ------------------------------------------------------
  Servo servo_break;  // create servo object to control a servo
  static byte servo_break_engaged_deg = 20;
  static byte servo_break_disengaged_deg = 0;
  unsigned long detach_servo_break_ts = 0;
  static int detach_servo_break_step = 100; // time in ms to allow for the servo to travel


// setup _________________________________________________________________________________________
void setup() {
  Serial.begin(115200); // changed from 9600
  randomSeed(analogRead(0));       // Generate a random sequence of numbers every time

  
 // engage servo break prior to session start
  servo_break.attach(pinServo_break); 
  Serial.print(11); Serial.print(" "); Serial.println(ts);      
  servo_break.write(servo_break_engaged_deg);

  delay(1500);
  servo_break.detach();

 // define inputs
  pinMode(pinFrame, INPUT);
  pinMode(pinRotaryEncoder01,INPUT_PULLUP);
  pinMode(pinRotaryEncoder02,INPUT_PULLUP);
  pinMode(pinLickometer, INPUT);

   if (!cap.begin(0x5A)) {
     Serial.println("MPR121 not detected!");
     while (1);
   }
  
 // define outputs
 
 
  pinMode(pinLaser, OUTPUT);
  pinMode(pinSpeaker, OUTPUT);
  pinMode(pinImageStart, OUTPUT);
  pinMode(pinImageStop, OUTPUT);

  pinMode(pinLaser_ttl, OUTPUT);
  
  
  pinMode(rotation_right_ttl, OUTPUT);
  pinMode(rotation_left_ttl, OUTPUT);

 

 // save starting state of rotary encoder
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoder02),fpinRotaryEncoder02,RISING); // set an interrupt on pinRotaryEncoder01, looking for a rising edge signal and executing the "pinRotaryEncoder01" Interrupt Service Routine (below)
  attachInterrupt(digitalPinToInterrupt(pinRotaryEncoder01),fpinRotaryEncoder01,RISING); // set an interrupt on pinRotaryEncoder02, looking for a rising edge signal and executing the "pinRotaryEncoder02" Interrupt Service Routine (below)

  // new stuff here
  maskA = digitalPinToBitMask(pinRotaryEncoder01);
  maskB = digitalPinToBitMask(pinRotaryEncoder02);
  maskAB = maskA | maskB;
  port = portInputRegister(digitalPinToPort(pinRotaryEncoder01));


  digitalWrite(pinLaser_ttl, LOW);   

   // wait for serial command before initating session---------------------------------------------------------------------
    while (Serial.available() <= 0) {} // wait for serial input to start session
  
   // save start time
  ts_start=millis();  
  digitalWrite(pinImageStart, HIGH);
  delay(100);
  digitalWrite(pinImageStart, LOW);

  
  
}



// _________________________________________________________________________________________________________________________________
/// void loop ______________________________________________________________________________________________________________________
// _________________________________________________________________________________________________________________________________
void loop() {
  // generate timestamp 
  ts=millis()-ts_start;

 // detach break servo ---------------------------
  if(ts >= detach_servo_break_ts && detach_servo_break_ts!=0){
    servo_break.detach();
    detach_servo_break_ts = 0;
  }

  // session initialization (runs once at start) -----------------------------------------------------------------
  if(session_end_ts == 0){
    session_end_ts = ts + session_duration;

    Serial.print(1);   Serial.print(" "); Serial.println(ts);                             // print start session
    Serial.print(100); Serial.print(" "); Serial.println(session_duration);               // print end ts
    Serial.print(101); Serial.print(" "); Serial.println(rotary_resolution);              // print rotary_resolution
    Serial.print(141); Serial.print(" "); Serial.println(tm_switch_pairing_step);         // print tm_switch_pairing_step
    Serial.print(131); Serial.print(" "); Serial.println(tone_freq_initially_paired);     // print tone_freq_initially_paired
    Serial.print(132); Serial.print(" "); Serial.println(tone_freq_initially_unpaired);   // print tone_freq_initially_unpaired

    Serial.print(140); Serial.print(" "); Serial.println(ts);                        // print time for session_pairing_current (ts must precede value)
    Serial.print(140); Serial.print(" "); Serial.println(session_pairing_current);   // print initial pairing

    Serial.print(142); Serial.print(" "); Serial.println(rotation_position);  // print initial position
    

   // start in unpaired side
    tone(pinSpeaker, tone_freq_initially_unpaired); 

   // disengage break
    servo_break.attach(pinServo_break); 
    Serial.print(12); Serial.print(" "); Serial.println(ts);      
    servo_break.write(servo_break_disengaged_deg);
    detach_servo_break_ts = ts + detach_servo_break_step;
  }
  

  // pairing switch
  if(tm_switch_pairing_ts == 0 && tm_switch_pairing_step != -1){
    tm_switch_pairing_ts = ts + tm_switch_pairing_step;
  }

  if(ts > tm_switch_pairing_ts && tm_switch_pairing_ts != 0){
    
    session_pairing_current = !session_pairing_current;

    if(toggle_initially_paired == 0){ // if in initially unpaired side 
      if(session_pairing_current){
        side_current_paired();
      } else {
        side_current_unpaired();
      }
    }

    if(toggle_initially_paired == 1){ // if in initially paired side
      if(session_pairing_current){
        side_current_unpaired();
      } else {
        side_current_paired();
      }
    }

    Serial.print(140); Serial.print(" "); Serial.println(ts);                        // print time for session_pairing_current (ts must precede value)
    Serial.print(140); Serial.print(" "); Serial.println(session_pairing_current);   // print session_pairing_current
    
    tm_switch_pairing_ts = ts + tm_switch_pairing_step;
  }

  
 // continuously collect frame time stamps
  frametimestamp();  


//////////// rotary encoder start
  // right //////////////////////////////////////////// 
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
    
  // left ////////////////////////////////////////////
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

// turn on / off laser based on position

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

  if(rotation_position <= ppr_encoded/2 && toggle_initially_paired == 1){
    tone(pinSpeaker, tone_freq_initially_unpaired); 
    Serial.print(28); Serial.print(" "); Serial.println(ts); // print entry to initially unpaired side

    if(session_pairing_current == 0){
      side_current_unpaired();
    } else {
      side_current_paired();
    }

    toggle_initially_paired = 0;
  } 

  if(rotation_position > ppr_encoded/2 && toggle_initially_paired == 0){
    tone(pinSpeaker, tone_freq_initially_paired); 
    Serial.print(27); Serial.print(" "); Serial.println(ts); // print entry to initially paired side
    
    if(session_pairing_current == 0){
      side_current_paired();
     } else {
      side_current_unpaired();
    } 

    
    toggle_initially_paired = 1;
  }
}

rotation_position_prev = rotation_position;
    
// turn off ttl pulses
  if(ts>=rotation_right_ttl_off && rotation_right_ttl_off!=0){
    
    digitalWrite(rotation_right_ttl,LOW);
    rotation_right_ttl_off = 0;
  }

  if(ts>=rotation_left_ttl_off && rotation_left_ttl_off!=0){
    digitalWrite(rotation_left_ttl,LOW);
    rotation_left_ttl_off = 0;
  }

  if(ts>=pinLickometer_ttl_off && pinLickometer_ttl_off!=0){
    digitalWrite(pinLickometer_ttl,LOW);
    pinLickometer_ttl_off = 0;
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
    digitalWrite(pinLaser, LOW);
    Serial.print(16); Serial.print(" "); Serial.println(ts); // print laser offset
}

void side_current_paired(){
    digitalWrite(pinLaser, HIGH);
    Serial.print(17); Serial.print(" "); Serial.println(ts); // print laser onset
}

void fpinRotaryEncoder02(){
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

void fpinRotaryEncoder01(){
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

/// frame time stamp _______________________________________________________________________________________________________
void frametimestamp() {
  pinFrame_read_previous = pinFrame_read;
  pinFrame_read = digitalRead(pinFrame);
  frame = pinFrame_read > pinFrame_read_previous;

  if (frame) {
    Serial.print(20); Serial.print(" "); Serial.println(ts); // print frame onset
  }

}

/// end session __________________________________________________________________________________________________________
void fun_end_session() {
  digitalWrite(pinImageStop, HIGH);
  servo_break.attach(pinServo_break);  
  servo_break.write(servo_break_engaged_deg);
  Serial.print(11); Serial.print(" "); Serial.println(ts);    
  
  delay(500);
  
  servo_break.detach();  
  digitalWrite(pinImageStop, LOW); 
  digitalWrite(pinLaser, LOW);
  noTone(pinSpeaker); 
  Serial.print(0); Serial.print(" "); Serial.println(ts);    // print end of session                 
  while(1){}                               //  Stops executing the program
}

  
 
