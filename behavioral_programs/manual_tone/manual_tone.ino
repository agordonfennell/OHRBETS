 // libraries
  #include <Servo.h>
 
 // servos 
  byte pinTone;
  unsigned long tone_freq;
  Servo servo_test;
  unsigned long baud = 115200;

  
void setup() {
  Serial.begin(baud); // start serial
  Serial.println("Serial Begin");
  Serial.println();

 // wait for spout id sent over serial before initiating script
  Serial.println("input pin for tone test");
  while (Serial.available() <= 0) {} // wait for serial input to start session
  pinTone = Serial.parseInt(); // read serial
  pinMode(pinTone, OUTPUT);
  noTone(pinTone);
  Serial.print("testing tone on pin: ");Serial.println(pinTone);
  Serial.println();
  Serial.println("send tone frequency over serial...");

}

void loop() {
   if(Serial.available()){
    tone_freq = Serial.parseInt(); // read serial

   // wait for input to begin calibration sequence
    if(tone_freq > 0){
      Serial.print("current tone frequency: ");Serial.println(tone_freq);
      tone(pinTone, tone_freq);
      Serial.println("");
      Serial.println("send new tone frequency over serial...");
        
    }
  }
}
