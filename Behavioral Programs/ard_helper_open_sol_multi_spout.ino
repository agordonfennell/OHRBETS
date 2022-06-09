
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <Servo.h>

//capicitance sensor
Adafruit_MPR121 cap = Adafruit_MPR121();

boolean open_mode = 0; // 0: touch to open set radial w/ serial, 1: serial to open
byte servo_radial_deg = 120;


static byte pinSol[]   = {5,6,7,8,9};
boolean stateSol[] = {0, 0, 0, 0, 0};
byte lick;
byte serial_lick;
byte lick_last;
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

Servo servo_radial;

static byte pinServo_radial = 41;
static byte pinServo_retract = 37;
static byte pinServo_break = 39;

byte serial_state = 0;

boolean serial_toggle = 0;

void setup() {
  // put your setup code here, to run once:
  
Serial.begin(115200);

for(uint8_t i = 0; i<=4; i++){
  pinMode(pinSol[i], OUTPUT);
}

pinMode(pinServo_retract, OUTPUT);
pinMode(pinServo_break, OUTPUT);  

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  servo_radial.attach(pinServo_radial);
  servo_radial.write(servo_radial_deg);
  delay(1000);
  servo_radial.detach();

  if(open_mode == 0){
    Serial.println("Send Serial to Update Radial Deg (0-180)");
  }
  
  if(open_mode == 1){
    Serial.println("Send Serial to Open Spouts (1-5)");
  }
}

void loop() {
   if(Serial.available() && open_mode == 1){
    serial_lick = Serial.parseInt();
    
      if(serial_lick > 0 && serial_lick <= 5){
        lick = serial_lick;
      }
   }
   
   if(Serial.available() && open_mode == 0){
      if(serial_state == 0){
        int servo_radial_deg = Serial.parseInt();
        if(servo_radial_deg > 0){
          Serial.print("Current Deg: "); Serial.println(servo_radial_deg);
          servo_radial.attach(pinServo_radial);
          servo_radial.write(servo_radial_deg);
          delay(500);
          servo_radial.detach();
        }
      }
   }

  
  //deliver reward if enough licks have been registered
    currtouched = cap.touched();

  // check to see if touch onset occured
  for (uint8_t i = 1; i <= 5; i++) { // for each sensor (change the maximum i if more touch sensors are added)
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) { // if touched now but not previously
      if(open_mode == 0){
        lick = i;                                               // flag lick
      }
    }
  }

  if(lick>0){
    stateSol[lick-1] = !stateSol[lick-1];
    digitalWrite(pinSol[lick-1],stateSol[lick-1]);

     Serial.print("Pin States: ");
    for(uint8_t i = 0; i<=4; i++){
      Serial.print(stateSol[i]);
    }
    Serial.println();
    
    lick = 0; // reset lick flag to close if statement
    
  }
  
  // save current state for comparision with next state
  lasttouched = currtouched;
}
