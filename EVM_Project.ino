#include <Keypad.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// ==========================================
// CONFIGURATION
// ==========================================

#define NUM_CANDIDATES 4
String candidateNames[NUM_CANDIDATES] = {"BJP", "CONGRESS", "AAP", "TMC"};

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

// Admin PIN (To check the final results at the end of the day or wipe data)
String adminPIN = "A000A";

const int VOTE_START_ADDR = 0; 
const int FLAG_START_ADDR = 100; 

// EEPROM Auto-Format Magic Signature
const int MAGIC_ADDR = 200;
const byte MAGIC_VAL = 0xA5; 

int voteCounts[NUM_CANDIDATES] = {0, 0, 0, 0};
enum State { STATE_INIT_LOCK, STATE_ENTER_PIN, STATE_OFFICER_AUTH, STATE_VOTING, STATE_ADMIN, STATE_FRAUD_LOCKOUT, STATE_ELECTION_CLOSED };
State currentState = STATE_INIT_LOCK;
String inputPIN = "";
int currentVoterIndex = -1;

void setup() {
  Serial.begin(9600);
  pinMode(officerButtonPin, INPUT); 
  
  Serial.println("\n\n===========================");
  Serial.println("System Booting..");

  // EEPROM AUTO-FORMAT
  if(EEPROM.read(MAGIC_ADDR) != MAGIC_VAL) {
    Serial.println("Performing first-time setup and formatting memory...");
    for(int i=0; i<NUM_CANDIDATES; i++) {
      EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), 0);
    }
    for(int i=0; i<NUM_VOTERS; i++) {
      EEPROM.write(FLAG_START_ADDR + i, 0);
    }
    EEPROM.write(MAGIC_ADDR, MAGIC_VAL);
    Serial.println("Memory format complete!");
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
  } else if (currentState == STATE_ADMIN) {
    handleAdmin();
  } else if (currentState == STATE_FRAUD_LOCKOUT) {
    handleFraudLockout();
  } else if (currentState == STATE_ELECTION_CLOSED) {
    // Election closed, do nothing, just wait.
    delay(100);
  }
}

void resetToInitLock() {
  currentState = STATE_INIT_LOCK;
  inputPIN = "";
  Serial.println("\n---------------------------");
  Serial.println("SYSTEM LOCKED.");
  Serial.println("Officer, please enter Initialization PIN (0000) to allow the next voter:");
}

void resetToPinEntry() {
  currentState = STATE_ENTER_PIN;
  inputPIN = "";
  currentVoterIndex = -1;
  Serial.println("\n---------------------------");
  Serial.println("Machine Unlocked. Waiting for Voter... Enter PIN on the Keypad:");
}

void resetToFraudLockout() {
  currentState = STATE_FRAUD_LOCKOUT;
  inputPIN = "";
  Serial.println("\n---------------------------");
  Serial.println("!!! SECURITY ALERT !!!");
  Serial.println("Multiple Vote Attempt Detected!");
  Serial.println("System Locked. Officer must enter Initialization PIN (0000) to resume:");
}

void closeElection() {
  currentState = STATE_ELECTION_CLOSED;
  Serial.println("\n==================================");
  Serial.println("ELECTION OFFICIALLY CLOSED");
  Serial.println("No further voting is permitted.");
  Serial.println("==================================");
  displayResults();
}

void handleInitLock() {
  char key = keypad.getKey();
  
  if (key) {
    if (key == '*' || key == '#') {
       inputPIN = "";
       Serial.println("\nPIN Cleared. Try again.");
       return;
    }
    
    inputPIN += key;
    Serial.print("*"); // Hide PIN for security
    
    if(inputPIN.length() == 4) {
      Serial.println();
      delay(500);
      
      if(inputPIN == officerPIN) {
        Serial.println("ELECTION UNLOCKED.");
        delay(1000);
        resetToPinEntry();
      } else if (inputPIN == closePIN) {
        closeElection();
      } else {
        Serial.println("ACCESS DENIED. Incorrect PIN.");
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
       Serial.println("\nPIN Cleared. Try again.");
       return;
    }
    
    inputPIN += key;
    Serial.print("*"); // Hide PIN for security
    
    if(inputPIN.length() == 4) {
      Serial.println();
      delay(500);
      
      if(inputPIN == officerPIN) {
        Serial.println("System Unlocked! Resuming Election...");
        delay(1000);
        resetToPinEntry();
      } else if (inputPIN == closePIN) {
        closeElection();
      } else {
        Serial.println("ACCESS DENIED. Incorrect PIN.");
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
       Serial.println("\nPIN Cleared. Try again.");
       return;
    }
    
    inputPIN += key;
    Serial.print(key); 
    
    if(inputPIN == adminPIN) {
      currentState = STATE_ADMIN;
      Serial.println("\n\n*** ADMIN MODE ***");
      delay(1000);
      displayResults();
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
        Serial.println("Invalid Voter PIN!");
        delay(2000);
        resetToPinEntry();
      } else {
        byte hasVoted = EEPROM.read(FLAG_START_ADDR + voterIdx);
        if(hasVoted == 1) {
          Serial.println("Error: Already Voted!");
          delay(1000);
          resetToFraudLockout(); // Trigger the security lockdown!
        } else {
          currentVoterIndex = voterIdx;
          currentState = STATE_OFFICER_AUTH;
          Serial.println("\n[OFFICER CHECK] Voter Authenticated.");
          Serial.println("-> To AUTHORIZE: Press the Physical Push Button (YES).");
          Serial.println("-> To REJECT: Type 'N' on this laptop keyboard and press Enter (NO).");
        }
      }
      inputPIN = ""; // Strictly clear buffer
    }
  }
}

void handleOfficerAuth() {
  // Check physical push button for YES
  if (digitalRead(officerButtonPin) == HIGH) {
    Serial.println("\n[OFFICER] Access GRANTED via Push Button.");
    currentState = STATE_VOTING;
    Serial.println("\n--- BALLOT UNLOCKED ---");
    for(int i=0; i<NUM_CANDIDATES; i++) {
      Serial.print("Press "); Serial.print(i+1); Serial.print(" for "); Serial.println(candidateNames[i]);
    }
    delay(500); // Debounce
    return;
  }
  
  // Check laptop keyboard for NO
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    if (incomingChar == 'N' || incomingChar == 'n') {
      Serial.println("\n[OFFICER] Access DENIED via Laptop Keyboard.");
      delay(2000);
      resetToInitLock(); // Send back to lock screen since this voter was rejected
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
    
    if (candidateIdx != -1) {
      voteCounts[candidateIdx]++;
      EEPROM.put(VOTE_START_ADDR + (candidateIdx * sizeof(int)), voteCounts[candidateIdx]); 
      
      EEPROM.write(FLAG_START_ADDR + currentVoterIndex, 1);
      
      Serial.println("\n*** VOTE CAST SUCCESSFULLY ***");
      Serial.print("You voted for: "); Serial.println(candidateNames[candidateIdx]);
      Serial.println("Thank You.");
      
      delay(3000);
      // REQUIRE OFFICER TO UNLOCK THE MACHINE FOR THE NEXT VOTER
      resetToInitLock(); 
    } else {
      Serial.println("Invalid Selection! Press 1, 2, 3, or 4 on the keypad.");
    }
  }
}

void displayResults() {
  Serial.println("\n--- ELECTION RESULTS ---");
  for(int i=0; i<NUM_CANDIDATES; i++) {
    Serial.print(candidateNames[i]); Serial.print(": "); Serial.println(voteCounts[i]);
  }
  Serial.println("------------------------");
  Serial.println("Press '#' to exit Admin Mode.");
  Serial.println("Press '*' to RESET all votes to zero.");
}

void handleAdmin() {
  char key = keypad.getKey();
  if(key) {
    if(key == '#') {
      resetToInitLock();
    } else if (key == '*') {
      Serial.println("\nResetting all data...");
      
      for(int i=0; i<NUM_CANDIDATES; i++) {
        voteCounts[i] = 0;
        EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), 0);
      }
      for(int i=0; i<NUM_VOTERS; i++) {
        EEPROM.write(FLAG_START_ADDR + i, 0);
      }
      
      delay(1000);
      Serial.println("Data Wiped!");
      displayResults();
    }
  }
}
