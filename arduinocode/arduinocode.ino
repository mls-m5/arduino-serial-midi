 int inputPin=2; //ECHO pin 
 int outputPin=4; //TRIG pin 

 void setup() 
 { 
     //  Set MIDI baud rate:
   //Serial.begin(31250);
   Serial.begin(9600); 
   pinMode(inputPin, INPUT); 
   pinMode(outputPin, OUTPUT);
 }
 
 void loop() 
 { 
   digitalWrite(outputPin, HIGH); //Trigger ultrasonic detection 
   delayMicroseconds(10);
   digitalWrite(outputPin, LOW); 
   int distance = pulseIn(inputPin, HIGH); //Read ultrasonic reflection
   distance= distance/58; //Calculate distance 
   Serial.println(distance); //Print distance 
   delay(10); 
 } 

//  plays a MIDI note.  Doesn't check to see that
//  cmd is greater than 127, or that data values are  less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}
