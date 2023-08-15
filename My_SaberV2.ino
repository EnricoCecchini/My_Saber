#include <FastLED.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Set number of LEDs in strip and PIN number
#define NUM_LEDS 144
#define LED_PIN 13

CRGB leds[NUM_LEDS];
CRGB color = CRGB::Red;

// BUTTONS
const int mainBTN_PIN = 12;
int mainBTN_STATE;
int mainBTN_STATE_PREV;
const int auxBTN_PIN = 4;
int auxBTN_STATE;
int auxBTN_STATE_PREV;

// Define debounce time
const unsigned long debounceTime = 50; // in milliseconds
unsigned long lastMainButtonChangeTime = 0;
unsigned long lastAuxButtonChangeTime = 0;

// MPU6050
// ARDUINO NANO PINS: SDA = A4, SCL = A5
Adafruit_MPU6050 mpu;
// Gyro
const int rotationThreshold = 34;
int prevGyroX = 0;
int currentGyroX = 0;
int prevGyroY = 0;
int currentGyroY = 0;
int prevGyroZ = 0;
int currentGyroZ = 0;
// Acceleration
int currentAccelX = 0;
int currentAccelY = 0;
int currentAccelZ = 0;

// Audio
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

static const uint8_t PIN_MP3_TX = 2; // D2: Connects to module's RX
static const uint8_t PIN_MP3_RX = 3; // D3: Connects to module's TX
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX); // RX, TX
DFRobotDFPlayerMini dfplayer;
bool speakerSetup = 0;

// Color List
String COLORS[] = {"RED", "BLUE", "GREEN", "PURPLE", "YELLOW", "WHITE", "TEAL", "ORANGE"};
int presetColors = 8;
int ledIndex = 0;
int ledChangeDuration = 3000;

// Effect List
String EFFECTS[] = {"SOLID", "PULSE", "RAINBOW", "FIRE"};
int effectNumber = 4;
int effectIndex = 0;

bool isOn = 0;
bool changedColor = 0;

// Counters
unsigned long currentTime = 0;

// Button Press counters
unsigned long mainButtonPressStart = 0;
unsigned long auxButtonPressStart = 0;

// LED speed counters
unsigned long changeLEDStart = 0;
int currentLED = 0;

// Pulse Effect variables
unsigned long lastPulseCounter = 0;
int pulseSpeed = 20;
bool pulseUp = true;
int pulseBrightness = 75;

// Other variables
bool setupComplete = 0;
unsigned long int cTest = 0;
bool colorChangeWarn = 0;

// FUNCTIONS

// UTILS
// Check MAIN button if pressed
void checkMainBTN() {
  // Update MAIN button state
  mainBTN_STATE_PREV = mainBTN_STATE;
  int newMainButtonState = digitalRead(mainBTN_PIN);

  // if (newMainButtonState != mainBTN_STATE && millis() - lastMainButtonChangeTime >= debounceTime) {
    lastMainButtonChangeTime = millis();
    mainBTN_STATE = newMainButtonState;

    // IF MAIN button pressed
    if (mainBTN_STATE == HIGH && mainBTN_STATE_PREV == HIGH) {
      //Serial.println("Pressed");

      // Turn ON saber if MAIN button is pressed and saber is OFF
      if (!isOn && mainButtonPressStart == 0) {
        turnOnSaber();
        isOn = 1;
        Serial.println("Turning On...");  
      }

      // Start MAIN button counter
      if (mainButtonPressStart == 0) {
        mainButtonPressStart = millis();
        Serial.println(String(mainButtonPressStart) + " Started");
      }

      // Print MAIN button press duration 
      if (mainButtonPressStart > 0 && (millis() - mainButtonPressStart)%1000 == 0) {
        Serial.println("MAIN: " + String((currentTime - mainButtonPressStart)/1000) + " seconds pressed");
      }
    }
  // }

  // IF MAIN button is pressed 5+ seconds, turn off saber
  if (isOn && (millis() - mainButtonPressStart) >= 5000 && mainButtonPressStart > 0) {
    Serial.println("Turning Off... ");
    turnOffSaber();
    isOn = 0;
  }
  
  // Reset MAIN button counter when released
  if (mainBTN_STATE == LOW && mainBTN_STATE_PREV == LOW && mainButtonPressStart > 0) {
    // IF MAIN button is released AFTER 3+ seconds, change effect
    if (isOn && (millis() - mainButtonPressStart) >= 3000 && (millis() - mainButtonPressStart) < 5000) {
      Serial.println("Change Effect...");
      changeSelectedEffect();
    }

    // Reset MAIN button counter on release
    if (mainButtonPressStart != 0)
      mainButtonPressStart = 0;
  }
}

// Check AUX button if pressed
void checkAuxBTN() {
  // Update AUX button state
  auxBTN_STATE_PREV = mainBTN_STATE;
  int newAuxButtonState = digitalRead(auxBTN_PIN);

  // if (newAuxButtonState != auxBTN_STATE && millis() - lastAuxButtonChangeTime >= debounceTime) {
    lastAuxButtonChangeTime = millis();
    auxBTN_STATE = newAuxButtonState;

    // IF MAIN button pressed
    if (auxBTN_STATE == HIGH && auxBTN_STATE_PREV == HIGH) {
      // Alert saber is OFF if attempt to change color
      if (!isOn && auxButtonPressStart == 0) {
        Serial.println("Turn on saber to change color");  
        // Play spark sound
      }

      // Start MAIN button counter
      if (auxButtonPressStart == 0) {
        auxButtonPressStart = millis();
      }

      // Print MAIN button press duration 
      if (auxButtonPressStart > 0 && (millis() - auxButtonPressStart)%1000 == 0) {
        Serial.println("AUX: " + String((millis() - auxButtonPressStart)/1000) + " seconds pressed");
      }
    }
  //}
  
  // IF AUX button is pressed 3+ seconds, change saber color
  if (isOn && !changedColor && (millis() - auxButtonPressStart) >= 5000 && auxButtonPressStart > 0) {
    Serial.println("Changing color...");

    // Change color
    changeSelectedColor();
    currentLED = 0;
    changedColor = 1;
  }
  
  // Reset AUX button counter when released
  if (auxBTN_STATE == LOW && auxBTN_STATE_PREV == LOW && auxButtonPressStart > 0) {
    // IF button is released AFTER 3+ seconds, change effect
    if (isOn && (millis() - auxButtonPressStart) < 500) {
      Serial.println("Clash Effect...");

      // Clash!
      clashEffect();
    }

    // Reset MAIN button counter on release
    if (auxButtonPressStart != 0)
      auxButtonPressStart = 0;

    changedColor = 0;
  }
}


// Change selected effect
void changeSelectedEffect() {
  effectIndex++;

  // Reset index
  if (effectIndex >= effectNumber) {
    effectIndex = 0;
  }

  if (EFFECTS[effectIndex] != "PULSE")
    FastLED.setBrightness(75);
}

// Change lightsaber color to preset
void changeSelectedColor() {

  ledIndex++;

  // Reset index
  if (ledIndex >= presetColors) {
    ledIndex = 0;
  }

  // Select Color
  switch (ledIndex) {
    case 1:
      color = CRGB::Blue;
      break;
    case 2:
      color = CRGB::Green;
      break;
    case 3:
      color = CRGB::Purple;
      break;
    case 4:
      color = CRGB::Yellow;
      break;
    case 5:
      color = CRGB(255, 255, 255);
      break;
    case 6:
      color = CRGB::Teal;
      break;
    case 7:
      color = CRGB::Orange;
      break;
    default:
      color = CRGB::Red;
      break;
  }

  Serial.println("Color changed to: " + String(COLORS[ledIndex]));
}

void changeColor() {

  // Check if all blade is selected color
  if (currentLED < NUM_LEDS) {
    Serial.println("Changing LED color: " + String(COLORS[ledIndex]));

    leds[currentLED] = color;
    FastLED.show();
    currentLED += 1;
  }
}

// EFFECTS
// Clash effect when AUX is pressed
void clashEffect() {

  // Play Sound
  dfplayer.play(4);
  delay(500);

  // Increase brightness
  FastLED.setBrightness(100);

  // Turn LEDs white
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
  }
  FastLED.show();

  delay(100);

  // Decrease brightness
  FastLED.setBrightness(75);

  // Turn LEDs back to selected color
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

// Pulse
void pulseEffect() {

  // Change brightness after 20ms
  if (millis() - lastPulseCounter >= pulseSpeed) {
    lastPulseCounter = millis();

    // Increase brightness until 255, then decrease until 0
    if (pulseUp) {
      pulseBrightness += 5;

      if (pulseBrightness >= 255)
        pulseUp = false;
    } else {
      pulseBrightness -= 5;

      if (pulseBrightness <= 0)
        pulseUp = true;
    }

    // Update brightness
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = color;
      leds[i].fadeToBlackBy(255 - pulseBrightness);
    }

    FastLED.show();

  }
}


// Turn ON saber
void turnOnSaber() {
  isOn = 1;
  dfplayer.play(1);
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    delay(10);
    FastLED.show();
  }
}

// Turn OFF saber
void turnOffSaber() {
  isOn = 0;
  dfplayer.play(5);
  delay(500);
  for (int i = NUM_LEDS-1; i >= 0; i--) {
    leds[i] = CRGB::Black;
    delay(10);
    FastLED.show();
  }
}


void setup() {
  Serial.begin(115200);

  // Init USB serial port for debugging
  Serial.begin(9600); // Init serial port for DFPlayer Mini
  softwareSerial.begin(9600);// Start communication with DFPlayer Mini

  // Start DFPlayer
  if (dfplayer.begin(softwareSerial)) {
    Serial.println("OK");
    // Set volume to maximum (0 to 30).
    dfplayer.volume(30);
    // Play the first MP3 file on the SD card
    //dfplayer.play(1);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!"); 
  } 

  // Start MPU6050;
  Wire.begin();

  Serial.println("Connecting to MPU6050...");

  // Try to initialize MPU6050!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050");
    while(true) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  // set gyro range to +- 500 deg/s
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);

  //delay(1000);

  pinMode(auxBTN_PIN, INPUT_PULLUP);
  pinMode(mainBTN_PIN, INPUT_PULLUP);

  //Serial.println("Setting up MAIN and AUX buttons");

  auxBTN_STATE = digitalRead(auxBTN_PIN);
  mainBTN_STATE = digitalRead(mainBTN_PIN);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(75);

  Serial.println("Setup Completed Succesfully!");

  setupComplete = 1;
  delay(1000);
}

void loop() {
  // Buttons
  mainBTN_STATE_PREV = mainBTN_STATE;
  auxBTN_STATE_PREV = auxBTN_STATE;
  mainBTN_STATE = digitalRead(mainBTN_PIN);
  auxBTN_STATE = digitalRead(auxBTN_PIN);
  //dfplayer.play(1);
  //Serial.println(dfplayer.readState());

  //Serial.println("Main: " + String(mainBTN_STATE));

  currentTime = millis();

  //Serial.println(currentTime - mainButtonPressStart);
  Serial.println(currentTime - mainButtonPressStart);

  // Gyro event
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  currentGyroX = g.gyro.x;
  currentGyroY = g.gyro.y;
  currentGyroZ = g.gyro.z;

  currentAccelX = a.acceleration.x;
  currentAccelY = a.acceleration.y;
  currentAccelZ = a.acceleration.z;

  // Twist to turn ON/OFF
  if (abs(currentGyroX) >= rotationThreshold) {
      if (!isOn) {
        //Serial.println("X: " + String(currentGyroX) + " \tY: " + String(currentGyroY) + " \tZ: " + String(currentGyroZ));
        Serial.println("Turning On with twist...");
        turnOnSaber();
        isOn = 1;
      } else {
        //Serial.println("X: " + String(currentGyroX) + " \tY: " + String(currentGyroY) + " \tZ: " + String(currentGyroZ));
        Serial.println("Turning On with twist...");
        turnOffSaber();
        isOn = 0;
      }
  }
  
  // Detect movement

  // Detect Clash


  if (cTest >= 50) {
    Serial.println(mainBTN_STATE + " " + mainBTN_STATE_PREV);
  }

  // Button Handler
  // If Saber is OFF and MAIN buttons is pressed, turn saber ON, else, start press counter
  if (mainBTN_STATE != mainBTN_STATE_PREV && currentTime >= 3000) {
    if (mainBTN_STATE == HIGH && !isOn) {
      turnOnSaber();
    } else if (mainBTN_STATE == HIGH && isOn && mainButtonPressStart < 100)
      mainButtonPressStart = currentTime;
    Serial.println("Pressed MAIN");
  }

  // If MAIN button is pressed for 5+ seconds, turn saber OFF, warn for color change after 3+ seconds
  if (mainBTN_STATE == HIGH && isOn && (currentTime - mainButtonPressStart) >= 5000 && mainButtonPressStart > 100) {
    Serial.println(currentTime - mainButtonPressStart);
    turnOffSaber();
  } else if (mainBTN_STATE == HIGH && isOn && (currentTime - mainButtonPressStart) >= 3000 && mainButtonPressStart > 100 && !colorChangeWarn) {
    clashEffect();
    clashEffect();
    colorChangeWarn = 1;
  }

  // If MAIN button is released after 3+ seconds, but less than 5, change color, if button is held less than 1 second show clash effect 
  if (mainBTN_STATE == LOW && (currentTime - mainButtonPressStart) >= 3000 && (currentTime - mainButtonPressStart) <= 5000 && mainButtonPressStart > 100) {
    ledIndex++;

    // Reset index
    if (ledIndex >= presetColors) {
      ledIndex = 0;
    }

    // Select Color
    switch (ledIndex) {
      case 1:
        color = CRGB::Blue;
        break;
      case 2:
        color = CRGB::Green;
        break;
      case 3:
        color = CRGB::Purple;
        break;
      case 4:
        color = CRGB::Yellow;
        break;
      case 5:
        color = CRGB(255, 255, 255);
        break;
      case 6:
        color = CRGB::Teal;
        break;
      case 7:
        color = CRGB::Orange;
        break;
      default:
        color = CRGB::Red;
        break;
    }

    Serial.println(COLORS[ledIndex]);
    currentLED = 0;
    mainButtonPressStart = 0;
    colorChangeWarn = 0;

  } else if (mainBTN_STATE == LOW && (currentTime - mainButtonPressStart) <= 1000 && mainButtonPressStart > 100) {
    clashEffect();
    mainButtonPressStart = 0;
  }

  if (mainBTN_STATE == LOW && mainButtonPressStart >= 100) {
    mainButtonPressStart = 0;
    //colorChangeWarn = 0;
  }

  // if (auxBTN_STATE != auxBTN_STATE_PREV && currentTime >= 3000) {
  //   if (auxBTN_STATE == HIGH && !isOn) {
  //     turnOnSaber();
  //     auxButtonPressStart = millis();
  //   } else if (auxBTN_STATE == HIGH && isOn)
  //     auxButtonPressStart = millis();
  //   Serial.println("Pressed AUX");
  // }

  // if (auxBTN_STATE != auxBTN_STATE_PREV) {
  //   if (auxBTN_STATE == HIGH && isOn && currentLED == 0) {
  //     //changeSelectedColor();
      
  //     ledIndex++;

  //     // Reset index
  //     if (ledIndex >= presetColors) {
  //       ledIndex = 0;
  //     }

  //     // Select Color
  //     switch (ledIndex) {
  //       case 1:
  //         color = CRGB::Blue;
  //         break;
  //       case 2:
  //         color = CRGB::Green;
  //         break;
  //       case 3:
  //         color = CRGB::Purple;
  //         break;
  //       case 4:
  //         color = CRGB::Yellow;
  //         break;
  //       case 5:
  //         color = CRGB(255, 255, 255);
  //         break;
  //       case 6:
  //         color = CRGB::Teal;
  //         break;
  //       case 7:
  //         color = CRGB::Orange;
  //         break;
  //       default:
  //         color = CRGB::Red;
  //         break;
  //     }

  //     Serial.println(COLORS[ledIndex]);
  //     currentLED = 0;
  //   }
  //   //Serial.println("Pressed AUX");
  // }

  // if (isOn)
  //   changeColor();

  // //Check if MAIN button is pressed
  // checkMainBTN();

  // // Check if AUX button is pressed
  // checkAuxBTN();

  // Change saber to selected color
  if (isOn && !changedColor && EFFECTS[effectIndex] == "SOLID")
    changeColor();
  
  // if (isOn) {
  //   switch(effectIndex) {
  //     case 1:
  //       //Serial.println("Pulse");
  //       pulseEffect();
  //       break;
  //     default:
  //       break;
  //   }
  // } 

  cTest += 1;
}
