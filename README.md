# ANS E-DoorLock

A simple electronic door lock system final project for our internship program, built with Arduino. It uses a keypad for password entry, a solenoid lock for the locking mechanism, and an OLED display for user feedback.

## Features

- 4-digit PIN code to unlock the door
- OLED display shows system status (e.g., "Enter PIN", "Access Granted", "Access Denied")
- Solenoid lock actuation on correct PIN entry
- Easy to customize PIN and wiring

## Hardware Components

| Component | Quantity | Notes |
|---|---|---|
| Arduino Board | 1 | Nano |
| 4x4 Keypad | 1 | |
| Solenoid Door Lock | 1 | 12V Preferably |
| OLED Display | 1 | Monochrome SSD1306 0.96' Display |
| Relay Module | 1 | 5v Single Channel |
| DC-DC Buck Converter | 1 | 12v to 5v Step down converter |
| Custom Board | - | Customized for the components |
| Power Supply | 1 | 12v Centralized PSU |



## Schematic

![Wiring Diagram](./schematic/wiring-diagram.svg)

> Follow the schematic to connect the keypad, OLED, and solenoid (via relay) to the Arduino before uploading the code.

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/nashzxc/ANS_E-DoorLock.git
   ```
2. Open the `door_lock.ino` file in the [Arduino IDE](https://www.arduino.cc/en/software).
3. Install required libraries 
   `Wire`,
   `Adafruit_GFX.h`, 
   `Adafruit_SSD1306.h`, 
   `Keypad`, 
   `EEPROM`, 
   via **Sketch > Include Library > Manage Libraries**.
5. Wire your components according to the schematic.
6. Select your board and port under **Tools**, then upload the sketch.

> Install the required libraries via the Arduino Library Manager. If any aren't available there, manual copies are included in this repo.

## Usage

1. Power on the device. The OLED will prompt you to **Enter PIN**.
2. Enter your 4-digit password using the keypad, and press **#** to submit.
3. Press **A** to clear the PIN you entered.
4. If correct, the solenoid unlocks the door and the OLED displays **Access Granted**.
5. If incorrect, the OLED displays **Access Denied** and prompts you to try again.

### Changing the Default PIN

The default PIN after flashing the firmware is "1234". Update it after uploading to save your new password at the EEPROM:

To change the PIN:

1. Press "C" at the keypad.
2. Enter the current PIN of the system (default is '1234').
3. Enter your new PIN and Confirm it by entering again.
4. Press "#" to save your PIN.

## Developers
Nash E. Claridad
Andrew P. Madriaga
..
..
