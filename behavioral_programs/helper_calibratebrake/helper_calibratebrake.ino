 // libraries
  #include <Servo.h>
 
 // parameters ************************************************************************************************
  static byte servo_brake_disengaged_deg = 0; // brake position at neutral; brake positions updated using serial
 // ***********************************************************************************************************
 
 // inputs
  static byte pinServo_retract = 37;
  static byte pinServo_brake = 39;
  static byte pinServo_radial = 41;
  
 // servos 
  Servo servo_retract;
  Servo servo_brake; 
  Servo servo_radial;
  static byte servo_radial_deg = 180; // multi-spout radial head position
  static byte servo_retract_retracted_deg = 120; // retractable spout position

 // variables
  byte servo_brake_engaged_deg;
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
    servo_brake.write(servo_brake_disengaged_deg);
    delay(250);
    servo_brake.detach();

   // radial
    servo_radial.attach(pinServo_radial);
    servo_radial.write(servo_radial_deg);
    delay(250);
    servo_radial.detach();
  
  // put your setup code here, to run once:
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_brake, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT);  


  Serial.println("send brake angle (0-180) over serial...");

}

void loop() {
   if(Serial.available()){
    servo_brake_engaged_deg = Serial.parseInt(); // read serial

   // wait for input to begin calibration sequence
    if(servo_brake_engaged_deg > 0 && servo_brake_engaged_deg <= 180){
      
      servo_brake.attach(pinServo_brake);
      servo_brake.write(servo_brake_disengaged_deg);
      delay(250);
      
      servo_brake.write(servo_brake_engaged_deg);
      delay(250);
      servo_brake.detach();
      
      Serial.print("current brake angle angle: ");Serial.println(servo_brake_engaged_deg);
      Serial.println("");
      Serial.println("send new brake angle (0-180) over serial...");
      
      if(servo_brake_engaged_deg > 180){
        Serial.println("error in servo_brake_engaged_deg input: value must be less than or equal to 180");
      }      
    }
  }
}
