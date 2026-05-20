#include <Wire.h>
#include <AS726X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "model.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

AS726X sensor;
Eloquent::ML::Port::SVM clf;

// ESP32 I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Rotary encoder pins
#define ROTARY_CLK 32
#define ROTARY_DT 33
#define ROTARY_SW 25

// Latest dataset meaning:
// 0 = UNINFESTED
// 1 = INFESTED
#define INFESTED_CLASS 1

// Threshold based on your actual latest readings
#define INFESTED_MAX_TOTAL 10000.0
#define UNINFESTED_MIN_TOTAL 11000.0

enum ScreenState {
  MAIN_MENU,
  RESULT_SCREEN,
  SENSOR_ERROR_SCREEN
};

ScreenState currentScreen = MAIN_MENU;
bool sensorReady = false;

void showTitle() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(2);
  display.setCursor(5, 22);
  display.println("LarvaSense");

  display.display();
}

void showClickToScan() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(2);
  display.setCursor(5, 10);
  display.println("LarvaSense");

  display.setTextSize(1);
  display.setCursor(28, 45);
  display.println("Click to Scan");

  display.display();
}

void showScanning() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(2);
  display.setCursor(12, 20);
  display.println("Scanning");

  display.setTextSize(1);
  display.setCursor(34, 45);
  display.println("Please wait");

  display.display();
}

void showSensorError() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(18, 12);
  display.println("Sensor Not Ready");

  display.setCursor(12, 30);
  display.println("Check power/wires");

  display.setCursor(30, 50);
  display.println("Click Again");

  display.display();
}

void showResult(int prediction, float totalSignal) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(2);

  if (prediction == INFESTED_CLASS) {
    display.setCursor(10, 10);
    display.println("Infested!");
  } else {
    display.setCursor(4, 10);
    display.println("Uninfested");
  }

  display.setTextSize(1);
  display.setCursor(25, 35);
  display.print("Total: ");
  display.println(totalSignal, 0);

  display.setCursor(35, 48);
  display.println("Click to");

  display.setCursor(30, 58);
  display.println("Scan Again");

  display.display();
}

bool rotaryButtonPressed() {
  if (digitalRead(ROTARY_SW) == LOW) {
    delay(50);

    if (digitalRead(ROTARY_SW) == LOW) {
      while (digitalRead(ROTARY_SW) == LOW) {
        delay(10);
      }

      delay(150);
      return true;
    }
  }

  return false;
}

bool initializeSensor() {
  if (sensorReady) {
    return true;
  }

  for (int i = 0; i < 10; i++) {
    Serial.print("Checking AS7263 sensor, attempt ");
    Serial.println(i + 1);

    if (sensor.begin()) {
      sensorReady = true;

      // NIR LIGHT ON
      sensor.setBulbCurrent(2);       // 0=12.5mA, 1=25mA, 2=50mA, 3=100mA
      sensor.enableBulb();

      sensor.setIndicatorCurrent(3);  // 0=1mA, 1=2mA, 2=4mA, 3=8mA
      sensor.enableIndicator();

      Serial.println("AS7263 sensor ready.");
      Serial.println("NIR light ON.");
      return true;
    }

    delay(500);
  }

  sensorReady = false;
  Serial.println("AS7263 sensor not detected.");
  return false;
}

int scanAndPredict(float &totalSignal) {
  // Make sure NIR light is ON before scanning
  sensor.setBulbCurrent(2);
  sensor.enableBulb();

  sensor.setIndicatorCurrent(3);
  sensor.enableIndicator();

  delay(500);

  sensor.takeMeasurements();

  float v610 = sensor.getCalibratedR();
  float v680 = sensor.getCalibratedS();
  float v730 = sensor.getCalibratedT();
  float v760 = sensor.getCalibratedU();
  float v810 = sensor.getCalibratedV();
  float v860 = sensor.getCalibratedW();

  totalSignal = v610 + v680 + v730 + v760 + v810 + v860;

  // Same order as your model.h training:
  // v610, v680, v730, v760, v810, v860
  float input[] = {v610, v680, v730, v760, v810, v860};

  int rawPrediction = clf.predict(input);
  int finalPrediction = rawPrediction;

  // Threshold correction based on your latest dataset and actual sensor readings
  if (totalSignal <= INFESTED_MAX_TOTAL) {
    finalPrediction = 1;  // INFESTED
  }
  else if (totalSignal >= UNINFESTED_MIN_TOTAL) {
    finalPrediction = 0;  // UNINFESTED
  }
  else {
    finalPrediction = rawPrediction;  // Use ML only in uncertain middle range
  }

  Serial.println("----- SCAN RESULT -----");
  Serial.print("610: "); Serial.print(v610);
  Serial.print(" | 680: "); Serial.print(v680);
  Serial.print(" | 730: "); Serial.print(v730);
  Serial.print(" | 760: "); Serial.print(v760);
  Serial.print(" | 810: "); Serial.print(v810);
  Serial.print(" | 860: "); Serial.print(v860);
  Serial.print(" | Total Signal: "); Serial.print(totalSignal);
  Serial.print(" | Raw ML Prediction: "); Serial.print(rawPrediction);
  Serial.print(" | Final Prediction: "); Serial.println(finalPrediction);

  if (finalPrediction == INFESTED_CLASS) {
    Serial.println("Displayed Result: INFESTED");
  } else {
    Serial.println("Displayed Result: UNINFESTED");
  }

  return finalPrediction;
}

void setup() {
  Serial.begin(115200);

  pinMode(ROTARY_CLK, INPUT_PULLUP);
  pinMode(ROTARY_DT, INPUT_PULLUP);
  pinMode(ROTARY_SW, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(0x3C, true)) {
    Serial.println("OLED not detected");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  showTitle();
  delay(2000);

  showClickToScan();
  currentScreen = MAIN_MENU;
}

void loop() {
  if (rotaryButtonPressed()) {

    if (currentScreen == MAIN_MENU) {
      showScanning();

      bool sensorOK = initializeSensor();

      if (!sensorOK) {
        showSensorError();
        currentScreen = SENSOR_ERROR_SCREEN;
        return;
      }

      float totalSignal = 0;
      int prediction = scanAndPredict(totalSignal);

      showResult(prediction, totalSignal);

      currentScreen = RESULT_SCREEN;
    }

    else if (currentScreen == RESULT_SCREEN || currentScreen == SENSOR_ERROR_SCREEN) {
      showTitle();
      delay(2000);

      showClickToScan();

      currentScreen = MAIN_MENU;
    }
  }
}