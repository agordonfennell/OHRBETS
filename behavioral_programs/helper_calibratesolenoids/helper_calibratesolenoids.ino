 // libraries
  #include <Servo.h>
 
 // parameters ************************************************************************************************
  static byte num_sol = 134; // 134 openings of 1.5µL should produce ~200µL of liquid              
  static byte isi = 140;     // inter-sol-time (~140 to approximate mouse lick rate)
  
  static byte servo_radial_deg = 180; // multi-spout radial head position
  static byte servo_retract_retracted_deg = 120; // retractable spout position
  static byte servo_brake_engaged_deg = 13; // brake position
  
 // ***********************************************************************************************************
 
 // inputs
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;
  

 // servos 
  Servo servo_retract;
  Servo servo_brake; 
  Servo servo_radial;

 // variables
  byte pinSol;    // pin for solenoid you wish to test (set once over serial upon startup)
  byte sol_duration_current;  // duration of solenoid openings (set repeatidly using serial after pinSol)
  volatile byte isi_current;
  
  byte serial_state = 0;
  boolean serial_toggle = 0;
  unsigned long baud = 115200;

  
void setup() {
  Serial.begin(baud); // start serial
  Serial.println("Serial Begin");
  Serial.println();

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

 // wait for spout id sent over serial before initiating script
  Serial.println("input pin for solenoid id");
  while (Serial.available() <= 0) {} // wait for serial input to start session
  pinSol = Serial.parseInt(); // read serial
  Serial.print("testing sol pin: ");Serial.println(pinSol);
  Serial.println();
  
  // put your setup code here, to run once:
  pinMode(pinSol, OUTPUT);
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT);  


  
  Serial.println("enter new duration of sol openings or restart serial to test new sol...");
  Serial.println();

}

void loop() {
  
   if(Serial.available()){
    sol_duration_current = Serial.parseInt(); // read serial

   // wait for input to begin calibration sequence
    if(sol_duration_current > 0 && sol_duration_current < isi){
      Serial.println("calibration sequence initiated with following parameters...");
      
      Serial.print("sol pin: "); Serial.println(pinSol);
      Serial.print("duration of sol openings: "); Serial.println(sol_duration_current);
      Serial.print("inter sol interval: "); Serial.println(isi);
      Serial.print("number of sol openings: "); Serial.println(num_sol);
      Serial.print("target vol(µL): "); Serial.println(num_sol * 1.5);
      Serial.println("");

      for(uint8_t i=1; i<=num_sol; i++){
        digitalWrite(pinSol, HIGH);
        delay(sol_duration_current);
        digitalWrite(pinSol, LOW);
        delay(isi);
        }
        Serial.println("calibration sequence complete; enter new duration of sol openings or restart serial to test new sol...");
      }

      if(sol_duration_current >= isi){
        Serial.println("error in sol_duration_current input: value be less than isi");
      }
      
    }

    

}
