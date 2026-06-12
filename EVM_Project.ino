#include <Keypad.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// ==========================================
// CONFIGURATION
// ==========================================

// --- Candidate Buttons ---
#define NUM_CANDIDATES 3
const int buttonPins[NUM_CANDIDATES] = {10, 11, 12}; 

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

// --- Voter Database ---
const int NUM_VOTERS = 5;
String validPINs[NUM_VOTERS] = {"1111", "2222", "3333", "4444", "5555"};
String adminPIN = "A000A";

// --- EEPROM Memory Addresses ---
const int VOTE_START_ADDR = 0; 
const int FLAG_START_ADDR = 100; 

int voteCounts[NUM_CANDIDATES] = {0, 0, 0};
enum State { STATE_ENTER_PIN, STATE_VOTING, STATE_ADMIN };
State currentState = STATE_ENTER_PIN;
String inputPIN = "";
int currentVoterIndex = -1;

void setup() {
  // Start the Serial Monitor to use the laptop as a screen!
  Serial.begin(9600);
  
  Serial.println("\n\n===========================");
  Serial.println("System Booting..");
  
  for(int i=0; i<NUM_CANDIDATES; i++) {
    pinMode(buttonPins[i], INPUT); 
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
  resetToPinEntry();
}

void loop() {
  wdt_reset(); 
  
  if(currentState == STATE_ENTER_PIN) {
    handlePinEntry();
  } else if (currentState == STATE_VOTING) {
    handleVoting();
  } else if (currentState == STATE_ADMIN) {
    handleAdmin();
  }
}

void resetToPinEntry() {
  currentState = STATE_ENTER_PIN;
  inputPIN = "";
  currentVoterIndex = -1;
  Serial.println("\n---------------------------");
  Serial.println("Enter Voter PIN on the Keypad:");
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
      return;
    }
    
    if(inputPIN.length() == 4) {
      Serial.println(); // move to next line
      delay(500); 
      
      int voterIdx = -1;
      for(int i=0; i<NUM_VOTERS; i++) {
        if(inputPIN == validPINs[i]) {
          voterIdx = i;
          break;
        }
      }
      
      if(voterIdx == -1) {
        Serial.println("Invalid PIN!");
        delay(2000);
        resetToPinEntry();
      } else {
        byte hasVoted = EEPROM.read(FLAG_START_ADDR + voterIdx);
        if(hasVoted == 1) {
          Serial.println("Error: Already Voted!");
          delay(2000);
          resetToPinEntry();
        } else {
          currentVoterIndex = voterIdx;
          currentState = STATE_VOTING;
          Serial.println("Access Granted!");
          delay(1000);
          Serial.println("\nSelect Candidate by pressing Button 1, 2, or 3...");
        }
      }
    }
    
    if(inputPIN.length() >= 5) {
      Serial.println("\nToo many digits!");
      resetToPinEntry();
    }
  }
}

void handleVoting() {
  static unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 200; 
  
  for(int i=0; i<NUM_CANDIDATES; i++) {
    int reading = digitalRead(buttonPins[i]);
    
    if (reading == HIGH) { 
      if ((millis() - lastDebounceTime) > debounceDelay) {
        
        voteCounts[i]++;
        EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), voteCounts[i]); 
        
        EEPROM.write(FLAG_START_ADDR + currentVoterIndex, 1);
        
        Serial.println("\nVote Cast Successfully!");
        Serial.println("Thank You.");
        
        lastDebounceTime = millis();
        delay(2000);
        resetToPinEntry();
        return; 
      }
    }
  }
}

void displayResults() {
  Serial.println("\n--- ELECTION RESULTS ---");
  Serial.print("Candidate 1: "); Serial.println(voteCounts[0]);
  Serial.print("Candidate 2: "); Serial.println(voteCounts[1]);
  Serial.print("Candidate 3: "); Serial.println(voteCounts[2]);
  Serial.println("------------------------");
  Serial.println("Press '#' to exit Admin Mode.");
  Serial.println("Press '*' to RESET all votes to zero.");
}

void handleAdmin() {
  char key = keypad.getKey();
  if(key) {
    if(key == '#') {
      resetToPinEntry();
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
