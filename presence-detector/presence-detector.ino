#include <NewPing.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Defining Sonar sensor 
#define TRIG_0 12
#define ECHO_0 13
#define TRIG_1 7
#define ECHO_1 8

#define iterations 5 //Number of readings in the calibration stage
#define MAX_DISTANCE 500 // Maximum distance (in cm) for the sensors to try to read.
#define DEFAULT_DISTANCE 45 // Default distance (in cm) is only used if calibration fails.
#define MIN_DISTANCE 15 // Minimum distance (in cm) for calibrated threshold.

// defining LED stuff
#define DATA_PIN            3
#define NUM_LEDS            11
#define BRIGHTNESS          15

// define the LCD stuff
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// defining logging
#define COUNT_LOGGING true
#define LED_LOGGING true

// sonar stuff
int16_t calibrate_0 = 0, calibrate_1 = 0; // The calibration in the setup() function will set these to appropriate values.
int16_t distances[2]; // These are the distances (in cm) that each of the Ultrasonic sensors read.
int8_t count = 0, limit = 3; //Occupancy limit should be set here: e.g. for maximum 8 people in the shop set 'limit = 8'.
int8_t flash_c = 0; // counter for flashing at max cap
bool flash_on = false; // decides if flashing is on or not.
bool direction = false; // true flips direction

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

NewPing sonar[2] = {   // Sensor object array.
  NewPing(TRIG_0, ECHO_0, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(TRIG_1, ECHO_1, MAX_DISTANCE)
};

// LED stuff
// Define the array of leds
CRGB leds[NUM_LEDS];

void clear_LED_strip() {
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
    FastLED.show();
  }
}

void LED_strip_enable(){
  if (LED_LOGGING){
    // Serial.println("Brigheness change called");
  }
  // we need to have percent of leds in relation to limit
  int8_t num_led_limit_ratio = (NUM_LEDS/limit);
  int8_t lit_leds = (num_led_limit_ratio*count);
  // then we should light up the matching lights
  for (int i = 0; i < NUM_LEDS; i++){
    // if occupancy == limit freak the f out
    if (count == limit){
      if (flash_on){
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
      }
    }else{
      // then if they are supposed to be lit, light them up
      if (i<lit_leds){
        leds[i] = CRGB::White;
      }else{
        leds[i] = CRGB::Black;
      }
    }
  }
  FastLED.show();
}

void write_to_LCD() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Count:");
  display.print(count);
  display.display();
}

void setRoomCap()
{
  int potentiometerValue = analogRead(A0);
  float percent = map(potentiometerValue , 0, 1023, 0, 94);
  int numberOfPeople = round(percent);
  new_limit = (numberOfPeople / 2) + 3; 
  new_direction = (numberOfPeople <= 47) ? true : false; 
  if (limit != new_limit) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Max Occupancy:");
    display.print(new_limit);
    display.setCursor(4, 0);
    display.print("Direction:");
    if (new_direction){
      display.print("<--")
    } else {
      display.print("-->")
    }
    display.display();
    delay(100);
    setRoomCap();
  }
  limit = new_limit;
  direction = new_direction;
} 


void setup() {
  // serial
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  // LED
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clear_LED_strip();
  // LCD 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Sup nerd!");
  display.display(); 
  // Sonar
  Serial.println("Calibrating...");
  delay(1500);
  for (int a = 0; a < iterations; a++) {
    delay(50);
    calibrate_0 += sonar[0].ping_cm();
    delay(50);
    calibrate_1 += sonar[1].ping_cm();
    delay(200);
  }
  calibrate_0 = 0.75 * calibrate_0 / iterations; //The threshold is set at 75% of the average of these readings. This should prevent the system counting people if it is knocked.
  calibrate_1 = 0.75 * calibrate_1 / iterations;

  if (calibrate_0 > MAX_DISTANCE || calibrate_0 < MIN_DISTANCE) { //If the calibration gave a reading outside of sensible bounds, then the default is used
    calibrate_0 = DEFAULT_DISTANCE;
  }
  if (calibrate_1 > MAX_DISTANCE || calibrate_1 < MIN_DISTANCE) {
    calibrate_1 = DEFAULT_DISTANCE;
  }

  // Serial.print("Entry threshold set to: ");
  // Serial.println(calibrate_0);
  // Serial.print("Exit threshold set to: ");
  // Serial.println(calibrate_1);

  delay(1000);
}

void loop() {
  Serial.print("Count: ");
  Serial.println(count);
  // inital pings
  distances[0] = sonar[0].ping_cm();
  delay(50); // Wait 40 milliseconds between pings. 29ms should be the shortest delay between pings.
  distances[1] = sonar[1].ping_cm();
  delay(50);
  if (COUNT_LOGGING){
    Serial.print("Distance 0: ");
    Serial.println(distances[0]);
    // Serial.print("Calibrate 0: ");
    // Serial.println(calibrate_0);
    Serial.print("Distance 1: ");
    Serial.println(distances[1]);
    // Serial.print("Calibrate 1: ");
    // Serial.println(calibrate_1);
  }

  
  // if sensor0 is triggered, wait and see if sensor1 is triggered for 20 cycles
  // if it is, do a status check
  if (distances[0] < calibrate_0 && distances[0] > 0) {
    Serial.println("TRIGGERED ENTRY CHECK");
    // now check to see if sensor1 is triggered a couple times
    for (int i = 0; i < 20; i++){
      distances[1] = sonar[1].ping_cm();
      if (COUNT_LOGGING){
        Serial.print("Distance 1: ");
        Serial.println(distances[1]);
        // Serial.print("Calibrate 1: ");
        // Serial.println(calibrate_1);
      }
      if (distances[1] < calibrate_1 && distances[1] > 0){
        if (direction) {
          count++;
        } else {
          if (count > 0) {
            count--;
          }
        }
        LED_strip_enable();
        write_to_LCD();
        delay(2000);
        break;
      }
      delay(80);
    }
  }
  
  // if sensor1 is triggered, wait and see if sensor0 is triggered for 20 cycles
  // if it is, do a status check
  if (distances[1] < calibrate_1 && distances[1] > 0) {
    Serial.println("TRIGGERED EXIT CHECK");
    // now check to see if sensor1 is triggered a couple times
    for (int i = 0; i < 20; i++){
      distances[0] = sonar[0].ping_cm();
      if (COUNT_LOGGING){
        Serial.print("Distance 0: ");
        Serial.println(distances[0]);
        // Serial.print("Calibrate 0: ");
        // Serial.println(calibrate_0);
      }
      if (distances[0] < calibrate_0 && distances[0] > 0){
        if (direction){
          if (count > 0){
            count--;
          }
        } else {
          count++;
        }
        LED_strip_enable();
        write_to_LCD();
        break;
        delay(2000);
      }
      delay(80);
    }
  }

  // Flashing logic
  if (flash_c == 5) {
    flash_c = 0; 
    flash_on = !flash_on;
  } else {
    flash_c += 1;
  } 
  LED_strip_enable();
}




