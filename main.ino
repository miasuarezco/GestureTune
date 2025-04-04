
#include <Wire.h>
#include <SPI.h>
#include <radio.h>
#include <TEA5767.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Melopero_APDS9960.h"

#define LED_PIN 3
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define FIX_BAND RADIO_BAND_FM
#define FIX_STATION 10390

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TEA5767 radio;
Melopero_APDS9960 device;

// Pins
int P1 = A0;
int B2 = A1;

// Radio variables
int freq = 10390;
bool radioMode = false; // false=Bluetooth, true=FM

// Timing controls
unsigned long lastGestureTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastFrequencyUpdate = 0;
const unsigned long GESTURE_COOLDOWN = 200;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100;
const unsigned long FREQUENCY_UPDATE_INTERVAL = 200;

const int COLOMBIAN_STATIONS[] = {
  9590,   // 95.9 FM - Radio Nacional de Colombia
  9090,   // 90.9 FM - La Mega
  9790,   // 97.9 FM - Radioacktiva
  10400,  // 104.0 FM - Los 40 Principales
  9990,   // 99.9 FM - W Radio
  9490,   // 94.9 FM - La FM
  10390,  // 103.9 FM - La X Más Música
  10290,  // 102.9 FM - Tropicana
  10490   // 104.9 FM - Vibra Bogotá
};


const char* STATION_NAMES[] = {
  "Nacional",
  "La Mega",
  "Radioactiva",
  "Los 40",
  "W Radio",
  "La FM",
  "La X",
  "Tropicana",
  "Vibra Bogotá"
};

const int NUM_STATIONS = sizeof(COLOMBIAN_STATIONS)/sizeof(COLOMBIAN_STATIONS[0]);
int currentStationIndex = 6; // Start at Tropicana 103.9

void setup() {
  // Pin initialization
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);

  // Display initialization
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;);
  }
  display.clearDisplay();
  display.display();
  delay(500);

  // Radio initialization
  radio.init();
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(2);
  radio.setMono(false);
  radio.setMute(true);
  radio.setSoftMute(true);

  // Gesture sensor setup
  gestureSetup();
  drawBlue();
}

void loop() {
  unsigned long currentMillis = millis();

  // Gesture handling
  if (currentMillis - lastGestureTime >= GESTURE_COOLDOWN) {
    device.updateGestureStatus();
    if (device.gestureFifoHasData) {
      handleGestures();
      lastGestureTime = currentMillis;
    }
  }

  // 2-second frequency updates (FM mode only)
  if (radioMode && (currentMillis - lastFrequencyUpdate >= FREQUENCY_UPDATE_INTERVAL)) {
    //radio.setFrequency(freq);
    lastFrequencyUpdate = currentMillis;
  }

  // Display management
  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    refreshDisplay();
    lastDisplayUpdate = currentMillis;
  }
}

void handleGestures() {
  device.parseGesture(100);
  
  if (device.parsedUpDownGesture == UP_GESTURE) {
    setMode(false); // Bluetooth mode
  }
  else if (device.parsedUpDownGesture == DOWN_GESTURE) {
    setMode(true); // FM mode
  }

  if (radioMode) {
    if (device.parsedLeftRightGesture == LEFT_GESTURE) {
      currentStationIndex = (currentStationIndex - 1 + NUM_STATIONS) % NUM_STATIONS;
      freq = COLOMBIAN_STATIONS[currentStationIndex];
      radio.setFrequency(freq);
    }
    else if (device.parsedLeftRightGesture == RIGHT_GESTURE) {
      currentStationIndex = (currentStationIndex + 1) % NUM_STATIONS;
      freq = COLOMBIAN_STATIONS[currentStationIndex];
      radio.setFrequency(freq);
    }
  }
}

void setMode(bool fmMode) {
  if (fmMode == radioMode) return;

  radioMode = fmMode;
  digitalWrite(7, fmMode ? LOW : HIGH);
  digitalWrite(8, fmMode ? LOW : HIGH);
  digitalWrite(9, fmMode ? LOW : HIGH);

  radio.setMute(!fmMode);
  radio.setSoftMute(!fmMode);
  radio.setVolume(fmMode ? 2 : 0);

  if(fmMode) radio.setFrequency(freq);
}

void refreshDisplay() {
  display.clearDisplay();
  if(radioMode) {
    drawFreq();
  } else {
    drawBlue();
  }
  display.display();
}

void drawFreq()
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  short offsetx=4;
  short offsety=16;
  display.clearDisplay();

  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,offsety+8);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  //display.write(dtostrf(freqo, 3, 1, buff));
  //display.print(s);
  //display.write("test");
  display.print(STATION_NAMES[currentStationIndex]);
  display.setCursor(0,offsety+  32);
  display.print(s);
  //display.print("MHz");
  
  display.display();
  delay(2500);
}
void drawBlue(){
  short offsetx=4;
  short offsety=16;
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,offsety+16);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.print("Bluetooth");
  display.display();
  delay(2500);
}

void gestureSetup() {
  if(device.initI2C(0x39, Wire) != NO_ERROR) while(true);
  if(device.reset() != NO_ERROR) while(true);
  
  device.enableGesturesEngine();
  device.setGestureProxEnterThreshold(25);
  device.setGestureGain(3);
  device.wakeUp();
}