#include <Keypad.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// ==========================================
// CONFIGURATION
// ==========================================

#define NUM_CANDIDATES 5
String candidateNames[NUM_CANDIDATES] = {"BJP", "CONGRESS", "AAP", "TMC", "NOTA"};

// --- Officer Control Button ---
// Connect this button to Pin 10. This is the Officer's "YES" button.
const int officerButtonPin = 10; 

// --- 4x4 Matrix Keypad ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Voter Database & Security ---
const int NUM_VOTERS = 5;
String validPINs[NUM_VOTERS] = {"1010", "2020", "3030", "4040", "5050"};

// Officer initialization PIN (To unlock the machine for EACH voter)
String officerPIN = "0000"; 

// Close Election PIN (To stop the election early)
String closePIN = "ABCD";

// Officer Reject PIN
String rejectPIN = "CCCC";

// Admin PIN (To check the final results at the end of the day)
String adminPIN = "A000A";

const int VOTE_START_ADDR = 0; 
const int FLAG_START_ADDR = 100; 

// EEPROM Auto-Format Magic Signature
const int MAGIC_ADDR = 200;
const byte MAGIC_VAL = 0xB2; 

int voteCounts[NUM_CANDIDATES] = {0, 0, 0, 0, 0};
enum State { STATE_INIT_LOCK, STATE_ENTER_PIN, STATE_OFFICER_AUTH, STATE_VOTING, STATE_FRAUD_LOCKOUT, STATE_ELECTION_CLOSED, STATE_WIPE_CONFIRM };
State currentState = STATE_INIT_LOCK;
String inputPIN = "";
int currentVoterIndex = -1;

void setup() {
  Serial.begin(9600);
  pinMode(officerButtonPin, INPUT); 
  
  Serial.println(F("\n\n==========================="));
  Serial.println(F("System Booting.."));

  // EEPROM AUTO-FORMAT
  if(EEPROM.read(MAGIC_ADDR) != MAGIC_VAL) {
    Serial.println(F("Performing first-time setup and formatting memory..."));
    for(int i=0; i<NUM_CANDIDATES; i++) {
      EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), 0);
    }
    for(int i=0; i<NUM_VOTERS; i++) {
      EEPROM.write(FLAG_START_ADDR + i, 0);
    }
    EEPROM.write(MAGIC_ADDR, MAGIC_VAL);
    Serial.println(F("Memory format complete!"));
  }
  
  for(int i=0; i<NUM_CANDIDATES; i++) {
    EEPROM.get(VOTE_START_ADDR + (i * sizeof(int)), voteCounts[i]);
    if(voteCounts[i] < 0 || voteCounts[i] > 1000) { 
      voteCounts[i] = 0;
      EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), voteCounts[i]);
    }
  }
  
  wdt_enable(WDTO_2S);
  delay(1000);
  resetToInitLock();
}

void loop() {
  wdt_reset(); 
  
  if (currentState == STATE_INIT_LOCK) {
    handleInitLock();
  } else if (currentState == STATE_ENTER_PIN) {
    handlePinEntry();
  } else if (currentState == STATE_OFFICER_AUTH) {
    handleOfficerAuth();
  } else if (currentState == STATE_VOTING) {
    handleVoting();
  } else if (currentState == STATE_FRAUD_LOCKOUT) {
    handleFraudLockout();
  } else if (currentState == STATE_ELECTION_CLOSED) {
    handleElectionClosed();
  } else if (currentState == STATE_WIPE_CONFIRM) {
    handleWipeConfirm();
  }
}

void resetToInitLock() {
  currentState = STATE_INIT_LOCK;
  inputPIN = "";
  Serial.println(F("\n---------------------------"));
  Serial.println(F("SYSTEM LOCKED."));
  Serial.println(F("Officer, please enter Initialization PIN to unlock the machine:"));
}

void resetToPinEntry() {
  currentState = STATE_ENTER_PIN;
  inputPIN = "";
  currentVoterIndex = -1;
  Serial.println(F("\n---------------------------"));
  Serial.println(F("Machine Unlocked. Waiting for Voter... Enter PIN on the Keypad:"));
}

void resetToFraudLockout() {
  currentState = STATE_FRAUD_LOCKOUT;
  inputPIN = "";
  Serial.println(F("\n---------------------------"));
  Serial.println(F("!!! SECURITY ALERT !!!"));
  Serial.println(F("Multiple Vote Attempt Detected!"));
  Serial.println(F("System Locked. Officer must enter Initialization PIN to resume:"));
}

void closeElection() {
  currentState = STATE_ELECTION_CLOSED;
  inputPIN = "";
  Serial.println(F("\n=================================="));
  Serial.println(F("ELECTION OFFICIALLY CLOSED"));
  Serial.println(F("No further voting is permitted."));
  Serial.println(F("=================================="));
  displayResults();
}

void handleInitLock() {
  char key = keypad.getKey();
  
  if (key) {
    if (key == '*' || key == '#') {
       inputPIN = "";
       Serial.println(F("\nPIN Cleared. Try again."));
       return;
    }
    
    inputPIN += key;
    Serial.print(F("*")); // Hide PIN for security
    
    if(inputPIN.length() == 4) {
      Serial.println();
      delay(500);
      
      if(inputPIN == officerPIN) {
        Serial.println(F("ELECTION UNLOCKED."));
        delay(1000);
        resetToPinEntry();
      } else if (inputPIN == closePIN) {
        closeElection();
      } else {
        Serial.println(F("\nACCESS DENIED. Incorrect PIN."));
        Serial.println(F("Please enter the correct PIN to unlock the machine:"));
      }
      inputPIN = ""; // Strictly clear buffer regardless
    }
  }
}

void handleFraudLockout() {
  char key = keypad.getKey();
  
  if (key) {
    if (key == '*' || key == '#') {
       inputPIN = "";
       Serial.println(F("\nPIN Cleared. Try again."));
       return;
    }
    
    inputPIN += key;
    Serial.print(F("*")); // Hide PIN for security
    
    if(inputPIN.length() == 4) {
      Serial.println();
      delay(500);
      
      if(inputPIN == officerPIN) {
        Serial.println(F("System Unlocked! Resuming Election..."));
        delay(1000);
        resetToPinEntry();
      } else if (inputPIN == closePIN) {
        closeElection();
      } else {
        Serial.println(F("\nACCESS DENIED. Incorrect PIN."));
        Serial.println(F("Please enter the correct PIN to resume the election:"));
      }
      inputPIN = ""; // Strictly clear buffer regardless
    }
  }
}

void handlePinEntry() {
  char key = keypad.getKey();
  
  if (key) {
    if (key == '*' || key == '#') {
       inputPIN = "";
       Serial.println(F("\nPIN Cleared. Try again."));
       return;
    }
    
    inputPIN += key;
    Serial.print(key); 
    
    if(inputPIN == adminPIN) {
      Serial.println(F("\n\n*** ADMIN MODE ***"));
      delay(1000);
      closeElection();
      inputPIN = "";
      return;
    }
    
    if(inputPIN.length() == 4) {
      Serial.println(); 
      delay(500); 
      
      if(inputPIN == closePIN) {
        closeElection();
        inputPIN = "";
        return;
      }
      
      int voterIdx = -1;
      for(int i=0; i<NUM_VOTERS; i++) {
        if(inputPIN == validPINs[i]) {
          voterIdx = i;
          break;
        }
      }
      
      if(voterIdx == -1) {
        Serial.println(F("Invalid Voter PIN!"));
        delay(2000);
        resetToPinEntry();
      } else {
        byte hasVoted = EEPROM.read(FLAG_START_ADDR + voterIdx);
        if(hasVoted == 1) {
          Serial.println(F("Error: Already Voted!"));
          delay(1000);
          resetToFraudLockout(); // Trigger the security lockdown!
        } else {
          currentVoterIndex = voterIdx;
          currentState = STATE_OFFICER_AUTH;
          inputPIN = ""; // clear buffer for the CCCC input
          Serial.println(F("\n[OFFICER CHECK] Voter Authenticated."));
          Serial.println(F("Physical verification is going on..."));
          Serial.println(F("-> Press Physical Push Button for YES"));
          Serial.println(F("-> Type Reject PIN on Keypad for NO"));
        }
      }
      inputPIN = ""; // Strictly clear buffer
    }
  }
}

void handleOfficerAuth() {
  // Check physical push button for YES
  if (digitalRead(officerButtonPin) == HIGH) {
    Serial.println(F("\n[OFFICER] Access GRANTED via Push Button."));
    currentState = STATE_VOTING;
    Serial.println(F("\n--- BALLOT UNLOCKED ---"));
    for(int i=0; i<NUM_CANDIDATES; i++) {
      Serial.print(F("Press ")); Serial.print(i+1); Serial.print(F(" for ")); Serial.println(candidateNames[i]);
    }
    delay(500); // Debounce
    return;
  }
  
  // Check Keypad for NO (CCCC)
  char key = keypad.getKey();
  if (key) {
    if (key == '*' || key == '#') {
       inputPIN = "";
       return;
    }
    
    inputPIN += key;
    Serial.print(key);
    
    if(inputPIN.length() == 4) {
      Serial.println();
      
      if(inputPIN == rejectPIN) {
        Serial.println(F("\n[OFFICER] Access DENIED via Keypad."));
        delay(2000);
        resetToInitLock(); 
      } else {
        inputPIN = ""; // Ignore incorrect pins in this state
      }
    }
  }
}

void handleVoting() {
  char key = keypad.getKey();
  
  if (key) {
    int candidateIdx = -1;
    
    if (key == '1') candidateIdx = 0;
    else if (key == '2') candidateIdx = 1;
    else if (key == '3') candidateIdx = 2;
    else if (key == '4') candidateIdx = 3;
    else if (key == '5') candidateIdx = 4;
    
    if (candidateIdx != -1) {
      voteCounts[candidateIdx]++;
      EEPROM.put(VOTE_START_ADDR + (candidateIdx * sizeof(int)), voteCounts[candidateIdx]); 
      
      EEPROM.write(FLAG_START_ADDR + currentVoterIndex, 1);
      
      Serial.println(F("\n*** VOTE CAST SUCCESSFULLY ***"));
      Serial.print(F("You voted for: ")); Serial.println(candidateNames[candidateIdx]);
      Serial.println(F("Thank You."));
      
      delay(3000);
      // REQUIRE OFFICER TO UNLOCK THE MACHINE FOR THE NEXT VOTER
      resetToInitLock(); 
    } else {
      Serial.println(F("Invalid Selection! Press 1, 2, 3, 4, or 5 on the keypad."));
    }
  }
}

void displayResults() {
  Serial.println(F("\n--- ELECTION RESULTS ---"));
  for(int i=0; i<NUM_CANDIDATES; i++) {
    Serial.print(candidateNames[i]); Serial.print(F(": ")); Serial.println(voteCounts[i]);
  }
  Serial.println(F("------------------------"));
  Serial.println(F("Press '#' to do nothing."));
  Serial.println(F("Press '*' to ERASE EVERYTHING and restart election."));
}

void handleElectionClosed() {
  char key = keypad.getKey();
  if(key) {
    if(key == '#') {
      Serial.println(F("Election is closed. Waiting..."));
    } else if (key == '*') {
      currentState = STATE_WIPE_CONFIRM;
      Serial.println(F("\n[WARNING] Are you sure you want to completely erase all votes?"));
      Serial.println(F("Press 'A' on Keypad to Confirm, or any other key to Cancel."));
    }
  }
}

void handleWipeConfirm() {
  char key = keypad.getKey();
  if(key) {
    if(key == 'A') {
      Serial.println(F("\nErasing all memory..."));
      
      for(int i=0; i<NUM_CANDIDATES; i++) {
        voteCounts[i] = 0;
        EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), 0);
      }
      for(int i=0; i<NUM_VOTERS; i++) {
        EEPROM.write(FLAG_START_ADDR + i, 0);
      }
      
      delay(1000);
      Serial.println(F("Memory Wiped! Restarting system."));
      delay(1000);
      resetToInitLock();
    } else {
      Serial.println(F("\nWipe Cancelled. Returning to results."));
      displayResults();
      currentState = STATE_ELECTION_CLOSED;
    }
  }
}
