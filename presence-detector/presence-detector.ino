#include <NewPing.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// Defining Sonar sensor 
#define TRIG_0 12
#define ECHO_0 13
#define TRIG_1 7
#define ECHO_1 8
// this is to flip the sonar readings
// #define TRIG_0 7
// #define ECHO_0 8
// #define TRIG_1 12
// #define ECHO_1 13

#define iterations 5 //Number of readings in the calibration stage
#define MAX_DISTANCE 500 // Maximum distance (in cm) for the sensors to try to read.
#define DEFAULT_DISTANCE 45 // Default distance (in cm) is only used if calibration fails.
#define MIN_DISTANCE 15 // Minimum distance (in cm) for calibrated threshold.

// defining LED stuff
#define DATA_PIN            3
#define NUM_LEDS            11
#define MAX_POWER_MILLIAMPS 5
#define BRIGHTNESS          15

// define the LCD stuff
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// defining logging
#define COUNT_LOGGING true
#define LED_LOGGING true

// sonar stuff
int16_t calibrate_0 = 0, calibrate_1 = 0; // The calibration in the setup() function will set these to appropriate values.
int16_t distance_0, distance_1; // These are the distances (in cm) that each of the Ultrasonic sensors read.
int8_t count = 0, limit = 4; //Occupancy limit should be set here: e.g. for maximum 8 people in the shop set 'limit = 8'.
bool prev_blocked0 = false, prev_blocked1 = false; //These booleans record whether the entry/exit was blocked on the previous reading of the sensor.

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

void write_to_LCD() {
  display.clearDisplay();
  display.print("Count:");
  display.print(count);
  display.display();
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

  Serial.print("Entry threshold set to: ");
  Serial.println(calibrate_0);
  Serial.print("Exit threshold set to: ");
  Serial.println(calibrate_1);

  delay(1000);
}

void LED_strip_enable(){
  if (LED_LOGGING){
    Serial.println("Brigheness change called");
  }
  // we need to have percent of leds in relation to limit
  int num_led_limit_ratio = (NUM_LEDS/limit);
  int lit_leds = (num_led_limit_ratio*count);
  // then we should light up the matching lights
  for (int i = 0; i <= NUM_LEDS; i++){
    // if occupancy == limit freak the f out
    if (count == limit){
      leds[i] = CRGB::Red;
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



void loop() {
  Serial.print("Count: ");
  Serial.println(count);
  // inital ping
  distance_0 = sonar[0].ping_cm();
  delay(50); // Wait 40 milliseconds between pings. 29ms should be the shortest delay between pings.
  distance_1 = sonar[1].ping_cm();
  delay(50);
  if (COUNT_LOGGING){
    Serial.print("Distance 0: ");
    Serial.println(distance_0);
    Serial.print("Distance 1: ");
    Serial.println(distance_1);
  }

  
  // if sensor0 is triggered, wait and see if sensor1 is triggered for 8 cycles
  // if it is, that is an entry
  if (distance_0 < calibrate_0 && distance_0 > 0) {
    if (prev_blocked0 == false) {
      Serial.println("TRIGGERED ENTRY CHECK");
      // now check to see if sensor1 is triggered a couple times
      for (int i = 0; i < 20; i++){
        distance_1 = sonar[1].ping_cm();
        if (COUNT_LOGGING){
          Serial.print("Distance 0: ");
          Serial.println(distance_0);
          Serial.print("Distance 1: ");
          Serial.println(distance_1);
        }
        if (distance_1 < calibrate_1 && distance_1 > 0){
          count++;
          LED_strip_enable();
          write_to_LCD();
          delay(3000);
          break;
        }
        delay(80);
      }
    }
    prev_blocked0 = true;
  } else {
    prev_blocked0 = false;
  }
  
  // // if sensor1 is triggered, wait and see if sensor0 is triggered for 8 cycles
  // // if it is, that is an exit
  // if (distance_1 < calibrate_1 && distance_1 > 0) {
  //   if (prev_blocked1 == false) {
  //     Serial.println("TRIGGERED EXIT CHECK");
  //     // now check to see if sensor1 is triggered a couple times
  //     for (int i = 0; i < 20; i++){
  //       distance_0 = sonar[0].ping_cm();
  //       if (COUNT_LOGGING){
  //         Serial.print("Distance 0: ");
  //         Serial.println(distance_0);
  //         Serial.print("Distance 1: ");
  //         Serial.println(distance_1);
  //       }
  //       if (distance_0 < calibrate_0 && distance_0 > 0){
  //         if (count > 0){
  //           count--;
  //           LED_strip_enable();
  //           write_to_LCD();
  //         }
  //         delay(3000);
  //         break;
  //       }
  //       delay(80);
  //     }
  //   }
  //   prev_blocked1 = true;
  // } else {
  //   prev_blocked1 = false;
  // }
}