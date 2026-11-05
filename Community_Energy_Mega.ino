#include <Arduino_GFX_Library.h>

// Arduino Mega 2560: ILI9341 TFT wiring
#define TFT_DC 49
#define TFT_CS 53
#define TFT_RST 48

// RGB565 colors
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_CYAN   0x07FF
#define C_GRAY   0x8410
#define C_ORANGE 0xFD20

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, TFT_RST, 1, false);

const uint8_t BUTTON_NORMAL_PIN = 22;
const uint8_t BUTTON_STRESS_PIN = 23;
const uint32_t STRESS_DURATION_MS = 30000UL;

enum DisplayMode { IDLE, NORMAL_DAY, GRID_STRAIN, DERMS_RESPONSE };
DisplayMode displayMode = IDLE;
uint32_t stressStartMs = 0;

struct DebouncedButton {
  uint8_t pin;
  bool lastRaw;
  bool stableState;
  uint32_t lastChangeMs;
};

DebouncedButton button1 = {BUTTON_NORMAL_PIN, HIGH, HIGH, 0};
DebouncedButton button2 = {BUTTON_STRESS_PIN, HIGH, HIGH, 0};

void beginButton(DebouncedButton &button) {
  pinMode(button.pin, INPUT_PULLUP);
  button.lastRaw = digitalRead(button.pin);
  button.stableState = button.lastRaw;
  button.lastChangeMs = millis();
}

bool wasPressed(DebouncedButton &button) {
  bool raw = digitalRead(button.pin);
  uint32_t now = millis();
  if (raw != button.lastRaw) {
    button.lastRaw = raw;
    button.lastChangeMs = now;
  }
  if ((now - button.lastChangeMs) >= 25 && raw != button.stableState) {
    button.stableState = raw;
    return (button.stableState == LOW);
  }
  return false;
}

void drawHeader(const char *title, const char *subtitle, uint16_t accent) {
  gfx->fillScreen(C_BLACK);
  gfx->fillRect(0, 0, 320, 51, 0x18E3);
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(10, 5);
  gfx->print(title);
  gfx->setTextSize(1);
  gfx->setTextColor(accent);
  gfx->setCursor(11, 32);
  gfx->print(subtitle);
}

void drawDemandChart(const uint8_t *samples, uint16_t lineColor) {
  const int16_t left = 20;
  const int16_t top = 65;
  const int16_t width = 280;
  const int16_t height = 105;
  gfx->drawRect(left, top, width, height, C_GRAY);

  for (int16_t i = 1; i < 4; i++) {
    int16_t y = top + (height * i) / 4;
    gfx->drawLine(left + 1, y, left + width - 2, y, 0x4208);
  }
  for (int16_t i = 1; i < 6; i++) {
    int16_t x = left + (width * i) / 6;
    gfx->drawLine(x, top + 1, x, top + height - 2, 0x4208);
  }

  int16_t previousX = left + 5;
  int16_t previousY = top + height - 8 - samples[0];
  for (uint8_t i = 1; i < 16; i++) {
    int16_t x = left + 5 + (i * (width - 10)) / 15;
    int16_t y = top + height - 8 - samples[i];
    gfx->drawLine(previousX, previousY, x, y, lineColor);
    previousX = x;
    previousY = y;
  }
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(left, top + height + 4);
  gfx->print("12 AM");
  gfx->setCursor(left + 124, top + height + 4);
  gfx->print("12 PM");
  gfx->setCursor(left + 248, top + height + 4);
  gfx->print("12 AM");
}

void showNormalDay() {
  static const uint8_t normalDemand[16] =
    {35, 32, 30, 31, 38, 48, 60, 70, 76, 78, 73, 66, 62, 67, 76, 65};
  drawHeader("CAISO OUTLOOK", "SIMULATED - NORMAL GRID DAY", C_CYAN);
  drawDemandChart(normalDemand, C_GREEN);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(24, 190);
  gfx->print("DEMAND TREND (ILLUSTRATIVE)");
  gfx->setCursor(24, 211);
  gfx->print("SUPPLY AVAILABLE  |  GRID STABLE");
}

void showGridStrain() {
  static const uint8_t strainDemand[16] =
    {56, 60, 65, 70, 74, 78, 81, 83, 85, 86, 87, 86, 85, 84, 82, 80};
  drawHeader("GRID STRAIN", "SIMULATED - HIGH DEMAND", C_ORANGE);
  drawDemandChart(strainDemand, C_RED);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(24, 190);
  gfx->print("DEMAND HIGH  |  RESERVES TIGHT");
  gfx->setCursor(24, 211);
  gfx->print("COMMUNITY RESPONSE IN PROGRESS");
}

void drawHouseIcon(int16_t x, int16_t y) {
  gfx->drawLine(x, y + 12, x + 15, y, C_ORANGE);
  gfx->drawLine(x + 15, y, x + 30, y + 12, C_ORANGE);
  gfx->drawRect(x + 4, y + 12, 23, 20, C_WHITE);
  gfx->fillRect(x + 12, y + 21, 7, 11, C_ORANGE);
}

void showDERMS() {
  drawHeader("DERMS RESPONSE", "SIMULATED - COMMUNITY ENERGY DISPATCH", C_GREEN);
  gfx->setTextSize(1);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(22, 66);
  gfx->print("HOMES SHARE STORED ENERGY");

  for (uint8_t i = 0; i < 5; i++) {
    drawHouseIcon(17 + i * 58, 92);
  }

  gfx->drawLine(32, 140, 286, 140, C_GREEN);
  for (uint8_t i = 0; i < 5; i++) {
    int16_t x = 54 + i * 50;
    gfx->drawLine(x, 140, x + 7, 136, C_GREEN);
    gfx->drawLine(x, 140, x + 7, 144, C_GREEN);
  }
  gfx->drawRect(55, 166, 210, 40, C_CYAN);
  gfx->setTextColor(C_CYAN);
  gfx->setTextSize(2);
  gfx->setCursor(77, 179);
  gfx->print("COMMUNITY GRID");
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(54, 218);
  gfx->print("HOME ENERGY SUPPORTING THE GRID");
}

void enterNormalMode() {
  displayMode = NORMAL_DAY;
  showNormalDay();
  Serial1.println("NORMAL");
}

void enterStressMode() {
  displayMode = GRID_STRAIN;
  stressStartMs = millis();
  showGridStrain();
  Serial1.println("STRESS");
}

void enterDERMSMode() {
  displayMode = DERMS_RESPONSE;
  showDERMS();
  Serial1.println("DERMS");
}

void setup() {
  beginButton(button1);
  beginButton(button2);
  Serial1.begin(9600); // Mega TX1 is pin 18
  gfx->begin(8000000);
  gfx->setRotation(3);
  gfx->fillScreen(C_BLACK); // neutral blank idle screen; no logo
  Serial1.println("IDLE");
}

void loop() {
  if (wasPressed(button1)) {
    enterNormalMode();
  }
  if (wasPressed(button2)) {
    enterStressMode();
  }

  if (displayMode == GRID_STRAIN &&
      (uint32_t)(millis() - stressStartMs) >= STRESS_DURATION_MS) {
    enterDERMSMode();
  }
}
