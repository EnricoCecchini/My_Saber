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

// BUTTONS
const int mainBTN_PIN = 12;
int mainBTN_STATE;
int mainBTN_STATE_PREV;
const int auxBTN_PIN = 4;
int auxBTN_STATE;
int auxBTN_STATE_PREV;

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
bool mainButtonPressed = false;
unsigned long mainButtonPressStart = 0;
unsigned long timeDiff = 0;
unsigned long currentTime = 0;
int long mainButtonPressDuration = 5000;

bool auxBTNPressed = false;
unsigned long auxBTNPressStart = 0;
int long auxBTNPressDuration = 5000;

// Animation parameters
int numMovingLeds = 4;
int animationDuration = 6; // milliseconds
int numAnimations = 6;
int animationDelay = 1; // milliseconds

// Acelerometer
Adafruit_MPU6050 mpu;


// FUNCTIONS

// Check AUX button
void checkAuxButton() {
  // Update button states
  auxBTN_STATE_PREV = auxBTN_STATE;
  auxBTN_STATE = digitalRead(auxBTN_PIN);

  Serial.println(isOn);
  Serial.println("AUX BTN " + String(auxBTN_STATE_PREV) + " " + String(auxBTN_STATE));

  // Check when AUX button is pressed
  if (auxBTN_STATE_PREV == HIGH && auxBTN_STATE == HIGH) {
      if (auxBTNPressStart == 0)
        auxBTNPressStart = millis();
        
      auxBTNPressed = true;
      Serial.println("Pressed " + String(auxBTNPressStart));
  } else {
    Serial.println("Pressed " + String(millis()));
    
    // If AUX button is released, after being pressed for <500 ms, make clash effect
    // IF AUX button is released, after being pressed >1000s, change color
    if (auxBTNPressStart != 0 && millis() - auxBTNPressStart < 500 && isOn) {
      Serial.println("Clash!");
      clashEffect();  
    } else if (auxBTNPressStart != 0 && (millis() - auxBTNPressStart > 1000) && isOn) {
      Serial.println("Changing Color after button press");
      Serial.println(ledIndex);
      changeColor();
    } else {
      auxBTNPressStart = 0;
    }
    Serial.println("Released");
  }

  delay(100);
}

// Check ON/OFF button is pressed
void checkMainBTN () {
  // Update button states
  mainBTN_STATE_PREV = mainBTN_STATE;
  auxBTN_STATE_PREV = auxBTN_STATE;

//  Serial.println(isOn);
//  Serial.println(String(mainBTN_STATE_PREV) + " " + String(mainBTN_STATE));
  
//  auxBTN_STATE = digitalRead(auxBTN_PIN);
  mainBTN_STATE = digitalRead(mainBTN_PIN);
  
  // Detect when ON/OFF button is pressed
  if (mainBTN_STATE_PREV == HIGH && mainBTN_STATE == HIGH) {
      if (mainButtonPressStart == 0)
        mainButtonPressStart = millis();
        
      mainButtonPressed = true;
      Serial.println("Pressed");

      // If ON/OFF button is pressed AND saber is OFF, turn saber ON 
      if (!isOn) {
        Serial.println("Turning On after button press");
        Serial.println(ledIndex);
        turnOnSaber();
      }
  }

  // Detect when ON/OFF button is released
  if (mainBTN_STATE_PREV == LOW && mainBTN_STATE == LOW) {
      mainButtonPressed = false;
      Serial.println("Released");

      Serial.println("Time Difference = " + String(currentTime - mainButtonPressStart));

      // If ON/OFF button was pressed less than 5 seconds, change effect
      // If ON/OFF button is pressed 5 seconds or more AND saber is ON, turn saber OFF
      if (currentTime - mainButtonPressStart < 5000 && isOn && mainButtonPressStart > 0) {
        Serial.println("Change effect");  

        if (effectIndex < 5) {
          effectIndex++;  
        } else {
          effectIndex = 0;  
        }
      } else if (mainButtonPressStart > 0 && currentTime - mainButtonPressStart >= 5000) {
        Serial.println("Turning Off after button press for 5 seconds");
        turnOffSaber();
      }

      mainButtonPressStart = 0;
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
      checkAuxButton();
      checkMainBTN();
      delay(pulseDuration / (maxBrightness - minBrightness) * pulseSpeed);
    }

    // Lower brightness of LEDs
    for (int brightness = maxBrightness; brightness >= minBrightness; brightness -= pulseSpeed) {
      for (int i = 0; i < nLeds; i++) {
        leds[i] = color;
        leds[i].fadeToBlackBy(maxBrightness - brightness);
      }  
      FastLED.show();
      checkAuxButton();
      checkMainBTN();
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
  checkAuxButton();
  checkMainBTN();
}

// Rainbow Effect Function
void rainbowEffect() {
  int hue = 0;

  for (int i = 0; i < 255; i++) {
    for (int j = 0; j < nLeds; j++) {
      leds[j] = CHSV(j - (i*2), 255, 255);
    }
    FastLED.show();
    checkAuxButton();
    checkMainBTN();
    
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

    if (isOn){
      checkMainBTN();
      checkAuxButton();
    }
    
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
  mainButtonPressed = 0;
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

  pinMode(auxBTN_PIN, INPUT_PULLUP);
  pinMode(mainBTN_PIN, INPUT_PULLUP);

  auxBTN_STATE = digitalRead(auxBTN_PIN);
  mainBTN_STATE = digitalRead(mainBTN_PIN);

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
  currentTime = millis();

  // Check if MAIN button is pressed
  checkMainBTN();

  // Check if AUX button is pressed
  checkAuxButton();

//  // Detect when AUX button is pressed
//  if (digitalRead(auxBTN_PIN) == HIGH && auxBTN_STATE == LOW && isOn) {
//    auxBTNPressStart = millis();
//    Serial.println("auxBTNPressStart");
////    Serial.println("Changing Color after button press");
////    Serial.println(ledIndex);
////    changeColor();
//  } else if (digitalRead(auxBTN_PIN) == LOW && auxBTN_STATE == HIGH && !isOn) {
//    Serial.println("Cannot change color when saber is off");
//  }
//
//  // Detect when AUX button is released
//  if (digitalRead(mainBTN_PIN) == HIGH && mainBTN_STATE == LOW) {
//      if (auxBTNPressStart != 0 && (currentTime - auxBTNPressStart < 5000)) {
//        Serial.println("Clash!");
//        clashEffect();
//      } else if (auxBTNPressStart != 0 && (currentTime - auxBTNPressStart >= 5000)) {
//        //auxBTNPressStart = millis();
//        Serial.println("Changing Color after button press");
//        Serial.println(ledIndex);
//        changeColor();
//      }
//
//      //auxBTNPressStart = 0;
//  }

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
  mainBTN_STATE = digitalRead(mainBTN_PIN);
  auxBTN_STATE = digitalRead(auxBTN_PIN);

//  Serial.println("mainBTN_STATE: " + String(mainBTN_STATE));
//  Serial.println("auxBTN_STATE: " + String(auxBTN_STATE));
}
