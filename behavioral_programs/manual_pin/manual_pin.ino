 // libraries
  #include <Servo.h>
 
 // parameters ************************************************************************************************
   static byte servo_radial_deg = 180; // multi-spout radial head position
   static byte servo_retract_retracted_deg = 120; // retractable spout position
   static byte servo_brake_engaged_deg = 15; // brake position
  
 // arduino servo pins (no manual adjustment)
  static byte servo_pins[] = {9,10,11};
  static byte n_servo_pins = 3;
  static byte pinServo_retract = 9; 
  static byte pinServo_brake = 10;
  static byte pinServo_radial = 11;

 // servos 
  Servo servo_retract;
  Servo servo_brake; 
  Servo servo_radial;

  static unsigned long baud = 115200;
  byte toggle_pin;
  boolean statePins[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  boolean skip_pin;
  
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

  for(byte i = 2; i<54; i++){
    for (byte i_servo_pin = 0; i_servo_pin < n_servo_pins; i_servo_pin++){
      if(i == servo_pins[i_servo_pin]){
        skip_pin = 1;
      }
    }

    if(!skip_pin){
      pinMode(i, OUTPUT);
    }
    
    skip_pin = 0;
  }

  Serial.println("enter pin  to toggle (2-53, excluding servos)...");
  Serial.println();
}

void loop() {
  if(Serial.available()){
    toggle_pin = Serial.parseInt(); // read serial
    
    // wait for input to begin calibration sequence
    if(toggle_pin > 1 && toggle_pin < 53){
      pinMode(toggle_pin, OUTPUT);
      digitalWrite(toggle_pin, HIGH);
  
      for (byte i_servo_pin = 0; i_servo_pin < n_servo_pins; i_servo_pin++){
        if(toggle_pin == servo_pins[i_servo_pin]){
          skip_pin = 1;
        }
      }
  
      if(!skip_pin){
        statePins[toggle_pin] = !statePins[toggle_pin];
        digitalWrite(toggle_pin, statePins[toggle_pin]);
        
        // print state of sols
        Serial.print("Input pin: "); Serial.println(toggle_pin);
        
        Serial.print("Pin States: ");
        
        for(byte i=2; i<54; i++){
          Serial.print(statePins[i]);
        }
        
        Serial.println();
    
        
      } else {
        Serial.println("error in input pin: must not match servo_pin");
      }
      
      skip_pin = 0;
    }
    Serial.println("");
  }
}
