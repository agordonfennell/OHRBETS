 // libraries
  #include <Servo.h>
 
 // servos 
  byte pinServo;
  byte servo_angle;
  Servo servo_test;
  unsigned long baud = 115200;

  
void setup() {
  Serial.begin(baud); // start serial
  Serial.println("Serial Begin");
  Serial.println();

 // wait for spout id sent over serial before initiating script
  Serial.println("input pin for servo test");
  while (Serial.available() <= 0) {} // wait for serial input to start session
  pinServo = Serial.parseInt(); // read serial
  pinMode(pinServo, OUTPUT);
  Serial.print("testing servo on pin: ");Serial.println(pinServo);
  Serial.println();
  servo_test.attach(pinServo);
  Serial.println("send servo angle (0-180) over serial...");

}

void loop() {
   if(Serial.available()){
    servo_angle = Serial.parseInt(); // read serial

   // wait for input to begin calibration sequence
    if(servo_angle > 0 && servo_angle <= 180){
      Serial.print("current servo angle angle: ");Serial.println(servo_angle);
      servo_test.write(servo_angle);
      delay(500);
      
      Serial.println("");
      Serial.println("send new servo angle (0-180) over serial...");
      
      if(servo_angle > 180){
        Serial.println("error in servo_angle input: value must be less than or equal to 180");
      }      
    }
  }
}
