#include <Wire.h>
#include <AS726X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Standard I2C address for SH1106 is often 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

AS726X sensor;

// ESP32 I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);

  // Initialize I2C for ESP32
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED
  if (!display.begin(0x3C, true)) {
    Serial.println("OLED not detected");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // Initialize AS7263 Sensor
  if (!sensor.begin()) {
    Serial.println("AS7263 not detected!");
    display.setCursor(0, 0);
    display.println("Sensor Error!");
    display.display();
    while (1);
  }

  // --- NIR LIGHT CONFIGURATION ---
  // Set Bulb current: 0=12.5mA, 1=25mA, 2=50mA, 3=100mA
  sensor.setBulbCurrent(2); 
  sensor.enableBulb(); // This turns ON the main NIR illumination LED

  // Optional: Also turn on the small indicator LED
  sensor.setIndicatorCurrent(3); // 0=1mA, 1=2mA, 2=4mA, 3=8mA
  sensor.enableIndicator();
  // -------------------------------

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.println("NIR Light: ON");
  display.display();

  delay(2000);
}

void loop() {
  // Take measurements from all 6 channels
  sensor.takeMeasurements();

  // Get calibrated values
  float v610 = sensor.getCalibratedR();
  float v680 = sensor.getCalibratedS();
  float v730 = sensor.getCalibratedT();
  float v760 = sensor.getCalibratedU();
  float v810 = sensor.getCalibratedV();
  float v860 = sensor.getCalibratedW();

  // Serial Output for Plotting/Debugging
  Serial.print(v610); Serial.print(",");
  Serial.print(v680); Serial.print(",");
  Serial.print(v730); Serial.print(",");
  Serial.print(v760); Serial.print(",");
  Serial.print(v810); Serial.print(",");
  Serial.println(v860);

  // Update OLED Display
  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("AS7263 NIR DATA");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setCursor(0, 15);
  display.print("610nm: "); display.println(v610, 1);
  display.print("680nm: "); display.println(v680, 1);
  display.print("730nm: "); display.println(v730, 1);
  display.print("760nm: "); display.println(v760, 1);
  display.print("810nm: "); display.println(v810, 1);
  display.print("860nm: "); display.println(v860, 1);

  display.display();

  delay(1000); // Wait 1 second before next reading
}