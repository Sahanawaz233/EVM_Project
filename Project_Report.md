# Electronic Voting Machine (EVM) Project Report

## 1. Project Overview
The objective of this project is to design and develop a highly secure, two-factor authenticated Electronic Voting Machine (EVM) using an Arduino Uno. The system replicates a real-world polling booth architecture, splitting the controls between a Polling Officer and a Voter to guarantee maximum electoral integrity.

## 2. Hardware Components
- **Microcontroller**: Arduino Uno.
- **Voter Interface**: 4x4 Membrane Matrix Keypad (For voter PIN entry, casting votes, and Admin access).
- **Officer Interface**: 1x Tactile Push Button (Used by the Officer to physically unlock the ballot).
- **Display Interface**: Serial Monitor via USB Communication.
- **Passive Components**: 10kΩ Resistor (Pull-down for signal stability).

## 3. Two-Factor Polling Station Architecture
The EVM operates on a strict multi-step state machine that requires both the voter and the election officer to be present:
1. **Officer Unlock**: The Officer must type an initialization PIN to unlock the machine for the next voter in line.
2. **Voter Authentication**: The voter enters a 4-digit PIN via the keypad. 
3. **Physical Verification**: Once the PIN is accepted, the Officer checks the voter's physical ID card. The Officer then presses the physical Push Button to unlock the ballot. (Alternatively, the Officer can type a reject PIN on the keypad to deny access).
4. **Vote Casting**: The voter presses a key (1-5) to vote for BJP, CONGRESS, AAP, TMC, or NOTA. The vote is permanently saved to EEPROM, and the machine instantly locks down again.

## 4. Addressing Judging Criteria

### 4.1 Security of the System
- **Two-Factor Authorization**: A valid voter PIN alone cannot cast a vote. The physical presence and button press of the Polling Officer is strictly required, making unauthorized voting impossible.
- **Prevention of multiple votes**: The system permanently binds a voter's PIN to an "Already Voted" flag stored in physical memory (`EEPROM`). A user cannot vote twice.
- **Active Fraud Lockdown**: If a voter attempts to vote twice, the machine does not just reject them—it triggers a massive Security Alert and completely locks down the polling booth until the Officer intervenes with a master PIN.
- **First-Boot Integrity Check**: The system includes a proactive EEPROM Auto-Format protocol. Upon booting, it checks a specific memory address for a cryptographic "magic signature." If the signature is missing, it assumes the memory is corrupted and forcefully wipes all EEPROM data to a clean zero state.
- **Data Privacy**: The vote tallies are entirely inaccessible from the outside. They can only be viewed by entering a highly secure Master Admin PIN, and the election can only be closed with a specific termination PIN.

### 4.2 Robustness
- **Pro-Level Memory Optimization**: All text displays are routed through the `F()` macro (e.g., `Serial.println(F("..."))`). This forces all strings to be stored in the Arduino's 32KB Flash Memory instead of its highly limited 2KB SRAM. This prevents heap fragmentation and guarantees the machine will never crash or freeze during a long election day.
- **Fault tolerance (Power failure handling)**: All votes and flags are mapped to non-volatile EEPROM. If the power cord is violently pulled during the election, zero data is lost.
- **Reliable operation (Hardware Debouncing)**: The Officer's physical unlock button utilizes a 10kΩ pull-down resistor tied to Ground. This ensures the digital pin reads a stable `LOW` (0V) state rather than "floating" and picking up ambient electromagnetic interference, guaranteeing no phantom unlocks.
- **Stability over time**: The system utilizes an internal hardware Watchdog Timer (`wdt_enable(WDTO_2S)`). If the system were to ever experience a fatal software freeze, the Watchdog detects the lockup and triggers an automatic hardware reset within 2 seconds.

### 4.3 Hardware Design
- **Compact Implementation**: By routing the entire voting process through a single Keypad and utilizing the Serial Monitor as a dynamic display, the physical footprint of the project is extremely minimal and neat.
- **Modular Control**: The architecture elegantly splits the hardware responsibilities. The breadboard push-button acts as the "Control Unit" for the Officer, while the Keypad acts as the "Ballot Unit" for the Voter.

## 5. Conclusion
This EVM successfully modernizes the voting process by utilizing standard microcontroller architecture to enforce strict, two-factor democratic security protocols. Through the strategic use of advanced memory management, EEPROM auto-formatting, Watchdog timers, and physical polling station workflows, it achieves maximum fault tolerance and system robustness, fully satisfying the requirements for an advanced electronic engineering project.
