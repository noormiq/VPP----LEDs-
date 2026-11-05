COMMUNITY ENERGY TWO-BOARD SKETCHES

FILES
- Community_Energy_Mega/Community_Energy_Mega.ino: reads the buttons, runs the simulated TFT views, and sends mode commands. The TFT starts on a blank black screen; there are no logos.
- Community_Energy_Uno/Community_Energy_Uno.ino: receives commands and animates the 82 WS2812B pixels.

INSTALL LIBRARIES
- Mega: Arduino_GFX_Library
- Uno: Adafruit_NeoPixel (install from Arduino IDE Library Manager)

WIRING
- Buttons stay on the Mega: yellow/common to GND; white/NO to D22 (Button 1) or D23 (Button 2). Use INPUT_PULLUP. Leave green/NC and red/black button lamp wires insulated.
- Mega TX1/D18 -> Uno D2 (SoftwareSerial receive).
- Mega GND -> Uno GND.
- Do not connect the Mega and Uno 5V pins together.
- Uno D6 -> 330-470 ohm series resistor -> WS2812B DIN.
- Power the LED strip from a regulated 5V supply. Connect its GND to Uno GND and Mega GND. Do not power the strip from the Uno 5V pin.
- Mega TFT: SCK D52, MOSI D51, MISO D50, CS D53, DC D49, RST D48. Rotation is set to 3.

BEHAVIOR
- Button 1: simulated normal-day TFT view and NORMAL LED animation.
- Button 2: simulated grid-strain TFT view and STRESS LED animation; after 30 seconds, both boards switch to DERMS response.
- Brightness output cap starts at 30/255 (~12% channel output) in the Uno sketch. Check the LED supply and wiring capacity before increasing it.

SLOWER ANIMATION TIMING
- Green path flow advances every 600 ms (50-pixel cycle ~30 seconds) with a fading six-pixel trail.
- Red grid-strain pulse takes 5 seconds per cycle.
- Homes begin 1.5 seconds apart; room LEDs turn on every 0.8 seconds.
- DERMS room highlight changes every 1.4 seconds.
