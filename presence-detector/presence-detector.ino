#include <NewPing.h>
#include <FastLED.h>

//Defining where the components are attached
#define TRIG_0 12
#define ECHO_0 13
#define TRIG_1 7
#define ECHO_1 8

#define iterations 5 //Number of readings in the calibration stage
#define MAX_DISTANCE 500 // Maximum distance (in cm) for the sensors to try to read.
#define DEFAULT_DISTANCE 15 // Default distance (in cm) is only used if calibration fails.
#define MIN_DISTANCE 15 // Minimum distance (in cm) for calibrated threshold.

#define DATA_PIN            3
#define NUM_LEDS            11
#define MAX_POWER_MILLIAMPS 5
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define BRIGHTNESS          30

#define COUNT_LOGGING true
#define LED_LOGGING true

// sonar stuff
float calibrate_0 = 0, calibrate_1 = 0; // The calibration in the setup() function will set these to appropriate values.
float distance_0, distance_1; // These are the distances (in cm) that each of the Ultrasonic sensors read.
int count = 0, limit = 5; //Occupancy limit should be set here: e.g. for maximum 8 people in the shop set 'limit = 8'.
bool prev_blocked0 = false, prev_blocked1 = false; //These booleans record whether the entry/exit was blocked on the previous reading of the sensor.

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

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clear_LED_strip();
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
    leds[i] = CRGB::White;
    FastLED.show();
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
}

void loop() {
  Serial.print("Count: ");
  Serial.println(count);
  // inital ping
  distance_0 = sonar[0].ping_cm();
  delay(40); // Wait 40 milliseconds between pings. 29ms should be the shortest delay between pings.
  distance_1 = sonar[1].ping_cm();
  delay(40);
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
          delay(1000);
          break;
        }
        delay(40);
      }
    }
    prev_blocked0 = true;
  } else {
    prev_blocked0 = false;
  }
  
  // if sensor1 is triggered, wait and see if sensor0 is triggered for 8 cycles
  // if it is, that is an exit
  if (distance_1 < calibrate_1 && distance_1 > 0) {
    if (prev_blocked1 == false) {
      Serial.println("TRIGGERED EXIT CHECK");
      // now check to see if sensor1 is triggered a couple times
      for (int i = 0; i < 20; i++){
        distance_0 = sonar[1].ping_cm();
        if (COUNT_LOGGING){
          Serial.print("Distance 0: ");
          Serial.println(distance_0);
          Serial.print("Distance 1: ");
          Serial.println(distance_1);
        }
        if (distance_0 < calibrate_0 && distance_0 > 0){
          if (count > 0){
            count--;
            LED_strip_enable();
          }
          delay(1000);
          break;
        }
        delay(40);
      }
    }
    prev_blocked1 = true;
  } else {
    prev_blocked1 = false;
  }
}