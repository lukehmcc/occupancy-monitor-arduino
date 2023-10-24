// Base of the code taken from here:
// https://lastminuteengineers.com/pir-sensor-arduino-tutorial/
// Thanks internet 

int inputPin0 = 7;               // choose the input pin (for PIR sensor)
int inputPin1 = 8;
int LEDPin0 = 12;
int LEDPin1 = 13;
int val0 = 0;                    // variable for reading the pin status
int val1 = 0;                    // variable for reading the pin status
int occState = 0;
int newVal0 = 0;
int newVal1 = 1;

void setup() {
  pinMode(inputPin0, INPUT); 
  pinMode(inputPin1, INPUT);
 
  Serial.begin(9600);
}

void loop() {
  newVal0 = digitalRead(inputPin0);
  newVal1 = digitalRead(inputPin1);

  if (newVal0 == LOW && val0 == HIGH) {
    for (int i = 0; i < 10; i++){
      if (newVal1 == 1){
        occState++;
        break;
      }
      delay(100);
    }
  }

  if (newVal1 == LOW && val1 == HIGH) {
    for (int i = 0; i < 10; i++){
      if (newVal0 == 1){
        if (occState > 0){
          occState--;
        }
        break;
      }
      delay(100);
    }
  }

  val0 = newVal0;
  val1 = newVal1;

  Serial.print("inputPin0: ");
  Serial.print(val0);
  Serial.print(" inputPin1: ");
  Serial.print(val1);
  Serial.print(" Occupancy: ");
  Serial.println(occState);

  delay(200); // Adjust delay as needed
}