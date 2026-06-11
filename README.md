# Electronic Voting Machine (EVM) Design Guide

This document outlines the step-by-step requirements and best practices to build an Electronic Voting Machine (EVM) using an Arduino Uno, specifically tailored to maximize your score on **Security**, **Robustness**, and **Hardware Design**.

---

## Phase 1: Hardware & Component Requirements

### Required Components
1. **Microcontroller**: Arduino Uno.
2. **Authentication Module (Crucial for Security)**:
   - **4x4 Matrix Keypad (Membrane)**: Used for voter authentication. Voters must enter a unique, secret PIN before the system unlocks to accept a vote.
3. **User Interface**:
   - **Push Buttons**: One for each candidate.
   - **Resistors (e.g., 10k Ohm)**: Use these as pull-up or pull-down resistors for the voting buttons to ensure stable voltage readings and prevent "floating" pin errors.
   - **I2C LCD Display (16x2 or 20x4)**: To display instructions (like "Enter PIN"), candidate names, and status without consuming too many Arduino pins.
   - **Buzzer & LEDs (Optional but recommended)**: For audio-visual feedback (e.g., beep when a keypad button is pressed).
4. **Power & Robustness**:
   - **9V/12V DC Adapter or Power Bank**: For a stable power supply.
   - **Capacitors (Optional)**: If you don't have these, it is completely fine! We will handle button debouncing perfectly using **software logic** (`millis()`), and the Arduino's built-in power regulation is stable enough.
5. **Hardware Design (Neatness)**:
   - **Protoboard / Zero PCB**: Do not use a breadboard for the final design; it is prone to loose connections.
   - **Header Pins & Jumper Wires**: For clean, organized connections.

---

## Phase 2: Addressing the Judging Criteria

### 1. Security of the System
* **Protection against unauthorized voting**: Implement a PIN-based lock system. The Arduino will prompt "Enter Voter PIN" on the LCD. The system remains completely locked and ignores the voting candidate buttons until a valid PIN is entered on the 4x4 keypad.
* **Prevention of multiple votes**: 
  - Maintain a database of valid PINs in the code (e.g., an array of passwords).
  - Keep track of which PINs have already been used by saving a "voted status flag" in the Arduino's **EEPROM**.
  - If a user enters a PIN that has already been flagged in the EEPROM, the LCD should display "Already Voted!" and reject the login.
* **Secure storage & Data Integrity**: 
  - Save the final vote tally directly to the Arduino's **EEPROM** (non-volatile memory) the exact moment a vote is cast. 
  - Keep the "Admin Mode" secure: An Admin can enter a special "Master PIN" (e.g., `*1234#`) on the keypad to view the vote tally. Only the admin should know this PIN.

### 2. Robustness
* **Fault tolerance (Power failure)**: Because votes (and "already voted" flags) are saved to EEPROM, if the power is suddenly cut, the data is perfectly safe. Upon restarting, the Arduino reads the EEPROM and restores the election state flawlessly.
* **Reliable operation without crashes**:
  - **Watchdog Timer (WDT)**: Enable the Arduino's internal Watchdog Timer. If the keypad reading hangs or the system freezes for any reason, the WDT will automatically reset the Arduino.
* **Debouncing**: 
  - The `Keypad.h` library naturally handles the debouncing for the 4x4 matrix. 
  - For the candidate voting buttons, you must use your resistors (as pull-downs) AND implement software debouncing (using `millis()`) to prevent a single button press from counting as multiple votes.

### 3. Hardware Design
* **Pin Management**: The 4x4 keypad will consume 8 digital pins (e.g., D2 to D9). Because the Uno only has 14 digital pins, using an **I2C module** for your LCD is absolutely critical here. I2C only requires 2 analog pins (A4, A5), leaving enough free pins for your candidate buttons.
* **Neat circuit design**: Solder components onto a custom PCB or a perfboard. The flat ribbon cable of the membrane keypad can be neatly routed and plugged into female header pins soldered to your board.

---

## Phase 3: Step-by-Step Implementation Plan

### Step 1: Prototyping
1. Connect the I2C LCD and test displaying "System Booting...".
2. Connect the 4x4 Matrix Keypad to 8 digital pins. Install the `Keypad.h` library and write a small test script to print pressed keys to the Serial Monitor.
3. Connect your candidate push buttons with your resistors acting as pull-down resistors.

### Step 2: Core Logic Integration
1. Write the logic to read a 4-digit PIN string from the keypad.
2. If the PIN matches a valid voter, display "Access Granted" and unlock the voting buttons for 10-15 seconds.
3. Wait for a candidate button press. Once pressed, increment the tally.
4. Immediately lock the system, display "Vote Cast", and return to the "Enter PIN" screen.

### Step 3: EEPROM & Robustness Integration
1. Modify the code to write the new vote count to a specific EEPROM address immediately after a vote is registered.
2. Write a flag (e.g., change a `0` to a `1`) in the EEPROM corresponding to the voter's PIN to permanently mark them as "voted".
3. Add logic in the `setup()` function to read these EEPROM addresses on startup so counts aren't lost if unplugged.

### Step 4: Admin Controls
1. Code a specific Master PIN (e.g., `A000A` using the A button on the keypad).
2. When the Master PIN is entered, bypass the voting loop and display the final results on the LCD.
3. Allow the admin to press the `*` or `#` key on the keypad while in Admin Mode to wipe the EEPROM and reset counts for a brand new election.

---

## Recommended Libraries
- `Keypad.h` (For the 4x4 Matrix Keypad)
- `Wire.h` and `LiquidCrystal_I2C.h` (For the LCD)
- `EEPROM.h` (For secure, non-volatile storage)
- `avr/wdt.h` (For the Watchdog Timer)
