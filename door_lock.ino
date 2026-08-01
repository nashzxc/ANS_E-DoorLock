#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <EEPROM.h>

//======================
// OLED (0.96" SSD1306, 128x64, I2C)
// NOTE: this is a monochrome display - it cannot show red/green.
// Color feedback below is done with a separate red/green LED instead,
// plus an inverted-display "alert" flash for wrong PIN.
//======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR    0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int HEADER_H   = 16;
const uint8_t BIG_SIZE = 4;
const int BIG_CHAR_W = 6 * BIG_SIZE;
const int BIG_CHAR_H = 8 * BIG_SIZE;

//======================
// Keypad
//======================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte colPins[ROWS] = {2,3,4,5};
byte rowPins[COLS] = {6,7,8,9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//======================
// Pins
//======================
const int relayPin = 10;
const int ledPin = 11;   // general status LED (unchanged behavior - on at boot)
const int redPin = 12;   // wrong PIN indicator - wire a red LED (+ resistor) here
const int greenPin = 13; // correct PIN indicator - Nano's built-in LED, or wire a green LED here

//======================
char password[5] = "1234";
char input[5] = "";
byte inputLen = 0;
const int EEPROM_ADDR = 0;
bool changeMode = false;
int changeStep = 0;
char newPass[5] = "";

// ---- Delayed masking ----
const unsigned long MASK_DELAY = 2000;
bool masked[4] = {false};
unsigned long digitTime[4] = {0};
// --------------------------

// ---- Blinking PIN (only while changing password) ----
bool blinkState = true;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 400; // ms on/off
// --------------------------------

//======================
void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(redPin, OUTPUT);
  digitalWrite(redPin, LOW);
  pinMode(greenPin, OUTPUT);
  digitalWrite(greenPin, LOW);

  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) { delay(100); }
  }

  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  display.clearDisplay();
  display.display();

  loadPasswordFromEEPROM();

  welcomeScreen();
}

//======================
void loop() {
  checkMasking();
  updateBlink();

  char key = keypad.getKey();
  if (key) {
    if (changeMode) {
      changePassword(key);
    } else {
      normalMode(key);
    }
  }
}

//======================
// OLED helpers
//======================
void clearHeader() {
  display.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, SSD1306_BLACK);
}

void clearBigArea() {
  display.fillRect(0, HEADER_H, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_H, SSD1306_BLACK);
}

void showHeader(const __FlashStringHelper *text) {
  clearHeader();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(text);
}

void showBig(const char *text) {
  clearBigArea();
  int len = strlen(text);
  if (len > 0) {
    int totalW = len * BIG_CHAR_W;
    int x = (SCREEN_WIDTH - totalW) / 2;
    if (x < 0) x = 0;
    int y = HEADER_H + ((SCREEN_HEIGHT - HEADER_H) - BIG_CHAR_H) / 2;
    display.setTextSize(BIG_SIZE);
    display.setCursor(x, y);
    display.print(text);
    display.setTextSize(1);
  }
}

// Full-screen centered status message. invert=true flashes it as a
// white-background/black-text "alert" look (closest a mono OLED can do to a color alarm).
void showCenteredMessage(const __FlashStringHelper *text, bool invert = false) {
  char buf[24];
  strncpy_P(buf, (const char *)text, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  int len = strlen(buf);
  uint8_t size = 2;
  if (len * 6 * size > SCREEN_WIDTH) size = 1;
  int w = len * 6 * size;
  int h = 8 * size;
  int x = (SCREEN_WIDTH - w) / 2;
  if (x < 0) x = 0;
  int y = (SCREEN_HEIGHT - h) / 2;

  display.clearDisplay();
  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
  display.setTextSize(1);
  display.invertDisplay(invert);
  display.display();
}

//======================
// Blinking PIN digits (change-password mode only)
//======================
void updateBlink() {
  if (!changeMode) return;
  unsigned long now = millis();
  if (now - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = now;
    blinkState = !blinkState;
    updateDisplay();
  }
}

void resetBlink() {
  blinkState = true;
  lastBlinkTime = millis();
}

//======================
// Helper functions for masking
//======================
void resetInputState() {
  input[0] = '\0';
  inputLen = 0;
  for (int i = 0; i < 4; i++) {
    masked[i] = false;
    digitTime[i] = 0;
  }
}

void updateDisplay() {
  if (changeMode && !blinkState) {
    clearBigArea();
    display.display();
    return;
  }
  char shown[5];
  for (int i = 0; i < inputLen; i++) {
    shown[i] = masked[i] ? '*' : input[i];
  }
  shown[inputLen] = '\0';
  showBig(shown);
  display.display();
}

void checkMasking() {
  bool needUpdate = false;
  unsigned long now = millis();
  for (int i = 0; i < inputLen; i++) {
    if (!masked[i] && (now - digitTime[i] >= MASK_DELAY)) {
      masked[i] = true;
      needUpdate = true;
    }
  }
  if (needUpdate) {
    updateDisplay();
  }
}

//======================
// EEPROM helpers
//======================
void savePasswordToEEPROM() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(EEPROM_ADDR + i, password[i]);
  }
}

void loadPasswordFromEEPROM() {
  char buf[5];
  bool valid = true;

  for (int i = 0; i < 4; i++) {
    char c = EEPROM.read(EEPROM_ADDR + i);
    if (c < '0' || c > '9') {
      valid = false;
    }
    buf[i] = c;
  }
  buf[4] = '\0';

  if (valid) {
    strcpy(password, buf);
  } else {
    strcpy(password, "1234");
    savePasswordToEEPROM();
  }
}

//======================
void welcomeScreen() {
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  display.invertDisplay(false);
  display.clearDisplay();
  showHeader(F("Enter PIN:"));
  showBig("");
  display.display();
  resetInputState();
}

//======================
void normalMode(char key) {
  if (key >= '0' && key <= '9') {
    if (inputLen < 4) {
      input[inputLen] = key;
      digitTime[inputLen] = millis();
      masked[inputLen] = false;
      inputLen++;
      input[inputLen] = '\0';
      updateDisplay();
    }
  } else if (key == 'A') {
    resetInputState();
    showCenteredMessage(F("Cleared"));
    delay(500);
    display.clearDisplay();
    showHeader(F("Enter PIN:"));
    showBig("");
    display.display();
  } else if (key == '#') {
    checkPassword();
  } else if (key == 'C') {
    changeMode = true;
    changeStep = 0;
    resetInputState();
    resetBlink();
    display.clearDisplay();
    showHeader(F("Old PIN:"));
    showBig("");
    display.display();
  }
}

//======================
void checkPassword() {
  if (strcmp(input, password) == 0) {
    digitalWrite(greenPin, HIGH);
    showCenteredMessage(F("ACCESS GRANTED"));
    digitalWrite(relayPin, LOW);
    delay(1000);
    digitalWrite(relayPin, HIGH);
    showCenteredMessage(F("Unlocked"));
    delay(3000);
    digitalWrite(greenPin, LOW);
  } else {
    digitalWrite(redPin, HIGH);
    showCenteredMessage(F("ACCESS DENIED"), true); // inverted = alert look
    delay(1500);
    display.invertDisplay(false);
    digitalWrite(redPin, LOW);
  }
  welcomeScreen();
}

//======================
void changePassword(char key) {
  if (key >= '0' && key <= '9') {
    if (inputLen < 4) {
      input[inputLen] = key;
      digitTime[inputLen] = millis();
      masked[inputLen] = false;
      inputLen++;
      input[inputLen] = '\0';
      resetBlink();
      updateDisplay();
    }
  } else if (key == 'A') {
    resetInputState();
    resetBlink();
    showBig("");
    display.display();
  } else if (key == '#') {
    if (changeStep == 0) {
      if (strcmp(input, password) == 0) {
        changeStep = 1;
        resetInputState();
        resetBlink();
        display.clearDisplay();
        showHeader(F("New PIN:"));
        showBig("");
        display.display();
      } else {
        digitalWrite(redPin, HIGH);
        showCenteredMessage(F("Wrong PIN"), true);
        delay(1500);
        display.invertDisplay(false);
        digitalWrite(redPin, LOW);
        changeMode = false;
        welcomeScreen();
      }
    } else if (changeStep == 1) {
      strcpy(newPass, input);
      resetInputState();
      resetBlink();
      changeStep = 2;
      display.clearDisplay();
      showHeader(F("Confirm:"));
      showBig("");
      display.display();
    } else if (changeStep == 2) {
      if (strcmp(input, newPass) == 0) {
        strcpy(password, newPass);
        savePasswordToEEPROM();
        digitalWrite(greenPin, HIGH);
        showCenteredMessage(F("PIN Saved"));
        delay(2000);
        digitalWrite(greenPin, LOW);
      } else {
        digitalWrite(redPin, HIGH);
        showCenteredMessage(F("Mismatch"), true);
        delay(1500);
        display.invertDisplay(false);
        digitalWrite(redPin, LOW);
      }
      changeMode = false;
      welcomeScreen();
    }
  }
}
