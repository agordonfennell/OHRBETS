  
  static byte pinSol = 6;        // { 4,  5,  6,  8,  9}
  static byte sol_duration = 22; // {28, 32, 22,  32, 26}
  static byte num_sol = 134;               
  static byte inter_sol_time = 140 - sol_duration; 
  
  static byte servo_radial_deg = 180;
  
  
  #include <Servo.h>

// servo retractable spout variables / parameters ------------------------------------------
  static byte pinServo_retract = 37;
  Servo servo_retract;
  static byte servo_retract_extended_deg = 180;

// servo rotary spout variables / parameters ------------------------------------------
  static byte pinServo_radial = 41;
  Servo servo_radial;
  volatile boolean update_radial_position;



  
void setup() {
  // put your setup code here, to run once:
  pinMode(pinSol, OUTPUT);

  // set initial spout position
  servo_radial.attach(pinServo_radial);
  servo_radial.write(servo_radial_deg);
  delay(500);
  servo_radial.detach();

  // fully extend spout prior to session start if using liquid reinforcer
  servo_retract.attach(pinServo_retract);
  servo_retract.write(servo_retract_extended_deg);
  delay(500);
  servo_retract.detach();

    // put your main code here, to run repeatedly:
  for(uint8_t i=1; i<=num_sol; i++){
    digitalWrite(pinSol, HIGH);
    delay(sol_duration);
    digitalWrite(pinSol, LOW);
    delay(inter_sol_time);
  }
}

void loop() {

}
