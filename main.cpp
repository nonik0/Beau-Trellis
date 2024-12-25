#include <Arduino.h>
#include <Adafruit_NeoTrellis.h>
#include <SPI.h>
#include "wifi_services.h"

#define SPEAKER_PIN 2
#define NUM_KEYS 16
#define NUM_ROWS 4
#define NUM_COLS 4
#define NUM_COLORS 4
#define OFF 0

Adafruit_NeoTrellis neoTrellis = Adafruit_NeoTrellis();
int buttonStates[NUM_KEYS];
WifiServices wifiServices;

uint32_t Off = neoTrellis.pixels.Color(0, 0, 0);
uint32_t Red = neoTrellis.pixels.Color(255, 0, 0);
uint32_t Orange = neoTrellis.pixels.Color(128, 128, 0);
uint32_t Yellow = neoTrellis.pixels.Color(255, 255, 0);
uint32_t Green = neoTrellis.pixels.Color(0, 255, 0);
uint32_t Blue = neoTrellis.pixels.Color(0, 0, 255);
uint32_t Violet = neoTrellis.pixels.Color(0, 255, 255);
uint32_t ColorValues[NUM_COLORS] = {Off, Blue, Green, Red};

int coordToIndex(int row, int col)
{
  return row * NUM_COLS + col;
}

int indexToRow(int index)
{
  return index / NUM_COLS;
}

int indexToCol(int index)
{
  return index % NUM_COLS;
}

void increment_key(int row, int col, int inc)
{
  if (row >= 0 && row < NUM_ROWS && col >= 0 && col < NUM_COLS)
  {
    int buttonIndex = coordToIndex(row, col);
    buttonStates[buttonIndex] = (buttonStates[buttonIndex] + inc) % NUM_COLORS;
    neoTrellis.pixels.setPixelColor(buttonIndex, ColorValues[buttonStates[buttonIndex]]);
    log_i("Button %d state: %d", buttonIndex, buttonStates[buttonIndex]);
  }
}

TrellisCallback buttonCallback(keyEvent evt)
{
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
  {
    int key = evt.bit.NUM;
    int row = indexToRow(key);
    int col = indexToCol(key);
    log_i("%d,%d pressed", row, col);
    increment_key(row, col, 2);
    increment_key(row - 1, col, 1);
    increment_key(row + 1, col, 1);
    increment_key(row, col - 1, 1);
    increment_key(row, col + 1, 1);
    neoTrellis.pixels.show();
    tone(SPEAKER_PIN, 1000 + key * 100);
  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
  {
    noTone(SPEAKER_PIN);
  }

  return NULL;
}

TrellisCallback wifiSetupCallback(keyEvent evt)
{
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && evt.bit.NUM == 0)
  {
    log_i("Button 0 is pressed, setting up wifi services");
    neoTrellis.pixels.setPixelColor(0, Blue);
    wifiServices.setup(DEVICE_NAME);
    wifiServices.createTask();

    // wait to connect, toggling color while waiting
    const unsigned long ConnectTimeoutMs = 10000;
    unsigned long startMs = millis();
    unsigned long lastToggleMillis = millis();
    bool isBlue = true;
    while (!wifiServices.isConnected() && millis() - startMs < ConnectTimeoutMs)
    {
      if (millis() - lastToggleMillis > 500)
      {
        lastToggleMillis = millis();
        neoTrellis.pixels.setPixelColor(0, isBlue ? Green : Blue);
        neoTrellis.pixels.show();
        isBlue = !isBlue;
      }
    }

    // indicate connection status
    neoTrellis.pixels.setPixelColor(0, wifiServices.isConnected() ? Green : Red);
    neoTrellis.pixels.show();
    delay(2000);
  }

  return NULL;
}

void setup()
{
  Serial.begin(115200);
  log_i("Starting setup...");

  neoTrellis.begin();
  neoTrellis.pixels.setBrightness(50);

  // initial wait for wifi setup if button 0 is pressed
  neoTrellis.activateKey(0, SEESAW_KEYPAD_EDGE_RISING);
  
  neoTrellis.registerCallback(0, wifiSetupCallback);

  const unsigned long ConnectPressTimeoutMs = 5000;
  unsigned long startMs = millis();
  unsigned long lastToggleMillis = millis();
  bool isOn = true;

  while (millis() - startMs < ConnectPressTimeoutMs)
  {
    neoTrellis.read();
    if (wifiServices.isConnected())
    {
      break;
    }
    else if (millis() - lastToggleMillis > 200)
    {
      lastToggleMillis = millis();
      neoTrellis.pixels.setPixelColor(0, isOn ? Blue : Off);
      neoTrellis.pixels.show();
      isOn = !isOn;
    }
    delay(20);
  }

  // now setup the rest of the button callbacks
  neoTrellis.unregisterCallback(0);
  neoTrellis.registerCallback(0, buttonCallback);
  neoTrellis.activateKey(0, SEESAW_KEYPAD_EDGE_FALLING);
  for (int i = 1; i < NUM_KEYS; i++)
  {
    neoTrellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    neoTrellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    neoTrellis.registerCallback(i, buttonCallback);
  }

  // initial state
  randomSeed(analogRead(0));
  for (int i = 0; i < NUM_KEYS; i++)
  {
    buttonStates[i] = random(NUM_COLORS);
    neoTrellis.pixels.setPixelColor(i, ColorValues[buttonStates[i]]);
  }
  neoTrellis.pixels.show();

  log_i("Setup complete");
}

void loop()
{
  neoTrellis.read();
  delay(20);
}
