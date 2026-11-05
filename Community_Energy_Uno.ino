#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <string.h>

// Mega TX1 pin 18 connects to this Uno receive pin D2.
SoftwareSerial megaLink(2, 3); // RX, TX; Uno D3 is unused

#define LED_PIN 6
#define LED_COUNT 82
#define LED_TYPE (NEO_GRB + NEO_KHZ800)

// Start with a capped output. Raise only after checking the 5 V supply,
// wire gauge, connectors, and strip current draw.
const uint8_t OUTPUT_CAP = 30; // 0-255, about 12% channel output
const uint16_t FRAME_MS = 40;
const uint16_t FLOW_STEP_MS = 600;      // green flow advances every 0.60 s
const uint16_t STRESS_PULSE_MS = 5000;  // one slow red pulse every 5 s
const uint16_t HOUSE_STAGGER_MS = 1500; // homes begin 1.5 s apart
const uint16_t ROOM_STEP_MS = 800;      // room LEDs turn on 0.8 s apart
const uint16_t DERMS_ROOM_MS = 1400;    // DERMS highlight changes slowly

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, LED_TYPE);

enum LedMode { LED_IDLE, LED_NORMAL, LED_STRESS, LED_DERMS };
LedMode ledMode = LED_IDLE;
uint32_t modeStartMs = 0;
uint32_t lastFrameMs = 0;

// User's LED numbers are 1-based; these arrays use Arduino's 0-based indexes.
const uint8_t pathPixels[50] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
  25,26,27,28,29,30,
  38,39,40,41,42,43,
  51,52,53,54,55,56,57,58,
  63,64,
  72,73,74,75,76,77,78,79,80,81
};

const uint8_t houseStart[5] = {18, 31, 44, 59, 65};
const uint8_t houseLength[5] = {7, 7, 7, 4, 7};

char commandBuffer[16];
uint8_t commandLength = 0;

uint8_t scaleChannel(uint8_t channel, uint8_t level) {
  uint16_t scaled = (uint16_t)channel * level / 255;
  return (uint8_t)scaled;
}

void setPixelScaled(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t level) {
  level = min(level, OUTPUT_CAP);
  strip.setPixelColor(index,
    scaleChannel(r, level),
    scaleChannel(g, level),
    scaleChannel(b, level));
}

void changeMode(LedMode newMode) {
  ledMode = newMode;
  modeStartMs = millis();
  strip.clear();
  strip.show();
}

void handleCommand(const char *command) {
  if (strcmp(command, "NORMAL") == 0) {
    changeMode(LED_NORMAL);
  } else if (strcmp(command, "STRESS") == 0) {
    changeMode(LED_STRESS);
  } else if (strcmp(command, "DERMS") == 0) {
    changeMode(LED_DERMS);
  } else if (strcmp(command, "IDLE") == 0) {
    changeMode(LED_IDLE);
  }
}

void readMegaCommands() {
  while (megaLink.available()) {
    char c = (char)megaLink.read();
    if (c == '\r') continue;
    if (c == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
    } else if (commandLength < sizeof(commandBuffer) - 1) {
      commandBuffer[commandLength++] = c;
    } else {
      commandLength = 0;
    }
  }
}

void drawGreenEnergyFlow(uint32_t now) {
  uint8_t head = (now / FLOW_STEP_MS) % 50;
  for (uint8_t k = 0; k < 50; k++) {
    uint8_t distance = (k + 50 - head) % 50;
    // A six-pixel brightness gradient makes the green light appear to flow.
    uint8_t level = 5;
    if (distance < 6) {
      level = 30 - distance * 5;
    }
    setPixelScaled(pathPixels[k], 0, 255, 28, level);
  }
}

void drawStressEnergyFlow(uint32_t now) {
  uint16_t halfPeriod = STRESS_PULSE_MS / 2;
  uint16_t phase = now % STRESS_PULSE_MS;
  uint8_t level;
  if (phase < halfPeriod) {
    level = 8 + (uint32_t)(OUTPUT_CAP - 8) * phase / halfPeriod;
  } else {
    level = OUTPUT_CAP - (uint32_t)(OUTPUT_CAP - 8) * (phase - halfPeriod) / halfPeriod;
  }
  for (uint8_t k = 0; k < 50; k++) {
    setPixelScaled(pathPixels[k], 255, 0, 0, level);
  }
}

void lightHousesStaggered(uint32_t elapsed, uint8_t level) {
  const uint16_t houseGapMs = HOUSE_STAGGER_MS;
  const uint16_t roomGapMs = ROOM_STEP_MS;
  const uint8_t orangeR = 255;
  const uint8_t orangeG = 118;
  const uint8_t orangeB = 18;

  for (uint8_t h = 0; h < 5; h++) {
    uint32_t houseElapsed = elapsed;
    uint32_t houseDelay = (uint32_t)h * houseGapMs;
    if (houseElapsed < houseDelay) continue;
    houseElapsed -= houseDelay;

    uint8_t lightsOn = 1 + houseElapsed / roomGapMs;
    if (lightsOn > houseLength[h]) lightsOn = houseLength[h];

    for (uint8_t n = 0; n < lightsOn; n++) {
      setPixelScaled(houseStart[h] + n, orangeR, orangeG, orangeB, level);
    }
  }
}

void drawDimDERMSHouses(uint32_t now) {
  // Keep each home softly lit while one room at a time brightens to suggest
  // staggered community dispatch. Different homes are offset in time.
  for (uint8_t h = 0; h < 5; h++) {
    for (uint8_t n = 0; n < houseLength[h]; n++) {
      setPixelScaled(houseStart[h] + n, 255, 118, 18, 30);
    }
    uint8_t activeRoom = ((now / DERMS_ROOM_MS) + h * 2) % houseLength[h];
    setPixelScaled(houseStart[h] + activeRoom, 255, 118, 18, 52);
  }
}

void renderFrame(uint32_t now) {
  strip.clear();

  if (ledMode == LED_NORMAL) {
    drawGreenEnergyFlow(now);
    lightHousesStaggered(now - modeStartMs, 34);
  } else if (ledMode == LED_STRESS) {
    drawStressEnergyFlow(now);
    for (uint8_t h = 0; h < 5; h++) {
      for (uint8_t n = 0; n < houseLength[h]; n++) {
        setPixelScaled(houseStart[h] + n, 255, 118, 18, OUTPUT_CAP);
      }
    }
  } else if (ledMode == LED_DERMS) {
    drawGreenEnergyFlow(now);
    drawDimDERMSHouses(now);
  }

  strip.show();
}

void setup() {
  megaLink.begin(9600);
  strip.begin();
  strip.clear();
  strip.show();
}

void loop() {
  readMegaCommands();
  uint32_t now = millis();
  if ((uint32_t)(now - lastFrameMs) >= FRAME_MS) {
    lastFrameMs = now;
    renderFrame(now);
  }
}
