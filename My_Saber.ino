#include <ezButton.h>
#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Set number of LEDs in strip and PIN number
#define NUM_LEDS 144
#define LED_PIN 13

CRGB leds[NUM_LEDS];
CRGB color = CRGB::Black;

int nLeds = 144;

const int onOfBTN_PIN = 12;
const int changeColorBTN_PIN = 4;
int changeColorBTN_STATE;
int onOfBTN_STATE;
int changeColorBTN_STATE_PREV;
int onOfBTN_STATE_PREV;

ezButton changeColorBTN(7);  // create ezButton object that attach to pin 7;
ezButton onOfBTN(4);  // create ezButton object that attach to pin 4;

// Color List
String COLORS[] = {"BLACK", "RED", "BLUE", "GREEN", "PURPLE", "YELLOW", "WHITE", "TEAL", "ORANGE"};
int presetColors = 9;
int ledIndex = 0;
int ledChangeDelay = 10;
int startLedChangeDelay = 10;

// Effect List
String EFFECTS[] = {"SOLID", "PULSE", "CLASH", "RAINBOW", "FIRE"};
int effectNumber = 5;
int effectIndex = -1;

// Effect Variables - PULSE
int maxBrightness = 255;
int minBrightness = 20;
int pulseSpeed = 5;

// Effect Variables - rainbow
int rainbowSpeed = 10;

// Manage On/Off saber
bool isOn = 0;
bool onOffButtonPressed = false;
unsigned long onOffButtonPressStart = 0;
unsigned long changeColorBTNPressStart = 0;
unsigned long timeDiff = 0;
unsigned long currentTime = 0;
bool longOnOffButtonPressDuration = 5000;

// Animation parameters
int numMovingLeds = 4;
int animationDuration = 6; // milliseconds
int numAnimations = 6;
int animationDelay = 1; // milliseconds

// Acelerometer
Adafruit_MPU6050 mpu;


// FUNCTIONS

// Check ON/OFF button is pressed
void checkOnOffBTN () {
    currentTime = millis();

  // Update button states
  onOfBTN_STATE_PREV = onOfBTN_STATE;
  changeColorBTN_STATE_PREV = changeColorBTN_STATE;

  Serial.println(isOn);
  Serial.println(String(onOfBTN_STATE_PREV) + " " + String(onOfBTN_STATE));
  
  changeColorBTN_STATE = digitalRead(changeColorBTN_PIN);
  onOfBTN_STATE = digitalRead(onOfBTN_PIN);
  
  // Detect when ON/OFF button is pressed
  if (onOfBTN_STATE_PREV == HIGH && onOfBTN_STATE == HIGH) {
      if (onOffButtonPressStart == 0)
        onOffButtonPressStart = millis();
        
      onOffButtonPressed = true;
      Serial.println("Pressed");

      // If ON/OFF button is pressed AND saber is OFF, turn saber ON 
      if (!isOn) {
        Serial.println("Turning On after button press");
        Serial.println(ledIndex);
        turnOnSaber();
      }
  }

  // Detect when ON/OFF button is released
  if (onOfBTN_STATE_PREV == LOW && onOfBTN_STATE == LOW) {
      onOffButtonPressed = false;
      Serial.println("Released");

      Serial.println("Time Difference = " + String(currentTime - onOffButtonPressStart));

      // If ON/OFF button was pressed less than 5 seconds, change effect
      // If ON/OFF button is pressed 5 seconds or more AND saber is ON, turn saber OFF
      if (currentTime - onOffButtonPressStart < 5000 && isOn && onOffButtonPressStart > 0) {
        Serial.println("Change effect");  

        if (effectIndex < 5) {
          effectIndex++;  
        } else {
          effectIndex = 0;  
        }
      } else if (onOffButtonPressStart > 0 && currentTime - onOffButtonPressStart >= 5000) {
        Serial.println("Turning Off after button press for 5 seconds");
        turnOffSaber();
      }

      onOffButtonPressStart = 0;
  }
}

// Function to produce humming sound
void humSound() {

}

// Function to produce swinging sound
void swingSound() {

}


// Effect Functions

// Pulse Effect Function
void pulseEffect() {
    int pulseDuration = 500;

    // Increase brightness of LEDs
    for (int brightness = minBrightness; brightness <= maxBrightness; brightness += pulseSpeed) {
      for (int i = 0; i < nLeds; i++) {
        leds[i] = color;
        leds[i].fadeToBlackBy(maxBrightness - brightness);
      }  
      FastLED.show();
      checkOnOffBTN();
      delay(pulseDuration / (maxBrightness - minBrightness) * pulseSpeed);
    }

    // Lower brightness of LEDs
    for (int brightness = maxBrightness; brightness >= minBrightness; brightness -= pulseSpeed) {
      for (int i = 0; i < nLeds; i++) {
        leds[i] = color;
        leds[i].fadeToBlackBy(maxBrightness - brightness);
      }  
      FastLED.show();
      checkOnOffBTN();
      delay(pulseDuration / (maxBrightness - minBrightness) * pulseSpeed);
    }
}

// Clash Effect Function
void clashEffect() {
    // Clash Effect on strike

  for (int i = 0; i < nLeds; i++) {
    leds[i] = CRGB::White;
  }

  FastLED.show();
  // Play sound
  delay(5);

  for (int i = 0; i < nLeds; i++) {
    leds[i] = color;
  }

  FastLED.show();
  checkOnOffBTN();
}

// Rainbow Effect Function
void rainbowEffect() {
  int hue = 0;

  for (int i = 0; i < 255; i++) {
    for (int j = 0; j < nLeds; j++) {
      leds[j] = CHSV(j - (i*2), 255, 255);
    }
    FastLED.show();
    checkOnOffBTN();
    
    delay(rainbowSpeed);
  }
}

// Fire Effect Function
void fireEffect() {
  
}

void moveLEDs() {
  for (int i = 0; i < NUM_LEDS - numMovingLeds; i++) {
    // Move the 4 LEDs from index i to i + numMovingLeds
    for (int j = 0; j < numMovingLeds; j++) {
      leds[i + j] = color; // You can change the color to your desired color
    }
    FastLED.show();
    delay(animationDuration);
    // Clear the LEDs to Black (off)
    for (int j = 0; j < numMovingLeds; j++) {
      leds[i + j] = CRGB::Black;
    }
  }
}

// Function Start Animation
void startAnimation() {
  Serial.println("Start Animation playing...");

  if (ledIndex == 0) {
    color = CRGB::Red;
    ledIndex = 1;
  }

  for (int i = 0; i < numAnimations; i++) {
    moveLEDs();
    delay(animationDelay);
    // Increase the animation speed for the next iteration
    animationDuration /= 2;
  }
  // Turn on the whole light strip after the animation
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color; // You can set the color to your desired color
  }
  FastLED.show();
  
  // Iterate LEDs, turning them ON one by one
  for (int i = 0; i < nLeds; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
  
// Function to Turn On saber
void turnOnSaber() {
  FastLED.setBrightness(75);
  
  Serial.println("Turning on...");
  Serial.println(ledIndex);

  if (ledIndex == 0) {
    color = CRGB::Red;
    ledIndex = 1;
  }

  // Iterate LEDs, turning them ON one by one
  for (int i = 0; i < nLeds; i++) {
    leds[i] = color;
    FastLED.show();

    if (isOn)
      checkOnOffBTN();
      
    delay(animationDuration);
  }

  isOn = 1;
}

// Function to Turn Off saber
void turnOffSaber() {
  Serial.println("Turning off...");

  // Iterate LEDs, turning them OFF one by one
  for (int i = nLeds -1; i >= 0; i--) {
    leds[i] = CRGB::Black;
    FastLED.show();
    delay(ledChangeDelay);
  }

  isOn = 0;
  onOffButtonPressed = 0;
}

// Function to change lightsaber color when button is pressed
void changeColor() {

  ledIndex++;

  // Reset index
  if (ledIndex >= presetColors) {
    ledIndex = 1;
  }

  // Select Color
  switch (ledIndex) {
    case 1:
      color = CRGB::Red;
      break;
    case 2:
      color = CRGB::Blue;
      break;
    case 3:
      color = CRGB::Green;
      break;
    case 4:
      color = CRGB::Purple;
      break;
    case 5:
      color = CRGB::Yellow;
      break;
    case 6:
      color = CRGB(255, 255, 255);
      break;
    case 7:
      color = CRGB::Teal;
      break;
    case 8:
      color = CRGB::Orange;
      break;
    default:
      color = CRGB::Black;
      break;
  }

  // Change color
  for (int i = 0; i < nLeds; i++) {
    leds[i] = color;
    FastLED.show();
    delay(ledChangeDelay);
  }

  Serial.println(COLORS[ledIndex]);
}


void setup() {
  //Serial.begin(9600);
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(changeColorBTN_PIN, INPUT_PULLUP);
  pinMode(onOfBTN_PIN, INPUT_PULLUP);

  changeColorBTN_STATE = digitalRead(changeColorBTN_PIN);
  onOfBTN_STATE = digitalRead(onOfBTN_PIN);
  
  changeColorBTN.setDebounceTime(1); // set debounce time to 50 milliseconds
  onOfBTN.setDebounceTime(50);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(75);
  
  // Start Accelerometer;
  Wire.begin();

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find Accelerometer");
    while (1) {
      delay(10);
      if (mpu.begin())
        break;
    }
  }
  Serial.println("Accelerometer Found!");

  
  //startAnimation();
}

void loop() {
  changeColorBTN.loop(); // MUST call the loop() function first
  onOfBTN.loop();

  // Check if On/OFF button is pressed
  checkOnOffBTN();

  // Detect when AUX button is pressed
  if (digitalRead(changeColorBTN_PIN) == HIGH && changeColorBTN_STATE == LOW && isOn) {
    changeColorBTNPressStart = millis();
    Serial.println("changeColorBTNPressStart");
//    Serial.println("Changing Color after button press");
//    Serial.println(ledIndex);
//    changeColor();
  } else if (digitalRead(changeColorBTN_PIN) == LOW && changeColorBTN_STATE == HIGH && !isOn) {
    Serial.println("Cannot change color when saber is off");
  }

  // Detect when AUX button is released
  if (digitalRead(onOfBTN_PIN) == HIGH && onOfBTN_STATE == LOW) {
      if (changeColorBTNPressStart != 0 && (currentTime - changeColorBTNPressStart < 5000)) {
        Serial.println("Clash!");
        clashEffect();
      } else if (changeColorBTNPressStart != 0 && (currentTime - changeColorBTNPressStart >= 5000)) {
        //changeColorBTNPressStart = millis();
        Serial.println("Changing Color after button press");
        Serial.println(ledIndex);
        changeColor();
      }

      //changeColorBTNPressStart = 0;
  }

  // If saber is OFF, turn off lights
  // If saber is ON show selected effect
  if (!isOn) {
    for (int i = 0; i < nLeds; i++) {
      leds[i] = CRGB::Black;
    }

    FastLED.show();
  } else {

    // Detect Motion

    // Detect Clash
    
    switch (effectIndex) {
      case 0:
        turnOnSaber();
        //Serial.println("Current Effect = " + EFFECTS[effectIndex]);
        break;
      case 1:
        pulseEffect();
        //Serial.println("Current Effect = " + EFFECTS[effectIndex]);
        break;
      case 2:
        FastLED.setBrightness(75);
        break;
      case 3:
        FastLED.setBrightness(75);
        rainbowEffect();
        //Serial.println("Current Effect = " + EFFECTS[effectIndex]);
        break;
      default:
        //turnOnSaber();
        //rainbowEffect();
        //Serial.println("Current Effect = " + EFFECTS[effectIndex]);
        break;
    }
  }

  // Update ON/OFF button state
  onOfBTN_STATE = digitalRead(onOfBTN_PIN);
  changeColorBTN_STATE = digitalRead(changeColorBTN_PIN);

//  Serial.println("onOfBTN_STATE: " + String(onOfBTN_STATE));
//  Serial.println("changeColorBTN_STATE: " + String(changeColorBTN_STATE));
}
