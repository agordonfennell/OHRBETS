 // libraries
  #include <Servo.h>
 
 // parameters ************************************************************************************************
  static byte servo_break_disengaged_deg = 0; // break position at neutral; break positions updated using serial
 // ***********************************************************************************************************
 
 // inputs
  static byte pinServo_retract = 37;
  static byte pinServo_break = 39;
  static byte pinServo_radial = 41;
  
 // servos 
  Servo servo_retract;
  Servo servo_break; 
  Servo servo_radial;
  static byte servo_radial_deg = 180; // multi-spout radial head position
  static byte servo_retract_retracted_deg = 120; // retractable spout position

 // variables
  byte servo_break_engaged_deg;
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
  
   // break
    servo_break.attach(pinServo_break);
    servo_break.write(servo_break_disengaged_deg);
    delay(250);
    servo_break.detach();

   // radial
    servo_radial.attach(pinServo_radial);
    servo_radial.write(servo_radial_deg);
    delay(250);
    servo_radial.detach();
  
  // put your setup code here, to run once:
  pinMode(pinServo_retract, OUTPUT);
  pinMode(pinServo_break, OUTPUT);  
  pinMode(pinServo_radial, OUTPUT);  


  Serial.println("send break angle (0-180) over serial...");

}

void loop() {
   if(Serial.available()){
    servo_break_engaged_deg = Serial.parseInt(); // read serial

   // wait for input to begin calibration sequence
    if(servo_break_engaged_deg > 0 && servo_break_engaged_deg <= 180){
      
      servo_break.attach(pinServo_break);
      servo_break.write(servo_break_disengaged_deg);
      delay(250);
      
      servo_break.write(servo_break_engaged_deg);
      delay(250);
      servo_break.detach();
      
      Serial.print("current break angle angle: ");Serial.println(servo_break_engaged_deg);
      Serial.println("");
      Serial.println("send new break angle (0-180) over serial...");
      
      if(servo_break_engaged_deg > 180){
        Serial.println("error in servo_break_engaged_deg input: value must be less than or equal to 180");
      }      
    }
  }
}
