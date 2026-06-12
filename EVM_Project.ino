#include <Keypad.h>
#include <LiquidCrystal.h> // Changed from LiquidCrystal_I2C
#include <EEPROM.h>
#include <avr/wdt.h>

// ==========================================
// CONFIGURATION
// ==========================================

// --- Candidate Buttons ---
// We are assuming 3 candidates for this example.
#define NUM_CANDIDATES 3
// Connect your push buttons to these digital pins.
// Make sure to use your 10k resistors as PULL-DOWN resistors.
const int buttonPins[NUM_CANDIDATES] = {10, 11, 12}; 

// --- Standard LCD Display (Parallel) ---
// Since the keypad uses 8 digital pins, we will use the Analog pins 
// for the LCD to save space. (Analog pins can work as digital pins!)
// Wiring: RS=A0, EN=A1, D4=A2, D5=A3, D6=A4, D7=A5
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5); 

// --- 4x4 Matrix Keypad ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// Connect Keypad Row pins 1, 2, 3, 4 to Arduino D9, D8, D7, D6
byte rowPins[ROWS] = {9, 8, 7, 6}; 
// Connect Keypad Column pins 1, 2, 3, 4 to Arduino D5, D4, D3, D2
byte colPins[COLS] = {5, 4, 3, 2}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Voter Database ---
const int NUM_VOTERS = 5;
// Unique 4-digit PINs assigned to each voter.
String validPINs[NUM_VOTERS] = {"1111", "2222", "3333", "4444", "5555"};

// --- Admin ---
// Master PIN to view results or reset the election
String adminPIN = "A000A";

// --- EEPROM Memory Addresses ---
const int VOTE_START_ADDR = 0; 
const int FLAG_START_ADDR = 100; 

// ==========================================
// GLOBAL VARIABLES
// ==========================================
int voteCounts[NUM_CANDIDATES] = {0, 0, 0};
enum State { STATE_ENTER_PIN, STATE_VOTING, STATE_ADMIN };
State currentState = STATE_ENTER_PIN;
String inputPIN = "";
int currentVoterIndex = -1;

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(9600);
  
  // 1. Initialize Standard LCD
  lcd.begin(16, 2); // Set up the LCD's number of columns and rows
  lcd.setCursor(0, 0);
  lcd.print("System Booting..");
  
  // 2. Initialize Buttons (Pull-down resistors assumed)
  for(int i=0; i<NUM_CANDIDATES; i++) {
    pinMode(buttonPins[i], INPUT); 
  }
  
  // 3. Read saved votes from EEPROM
  for(int i=0; i<NUM_CANDIDATES; i++) {
    EEPROM.get(VOTE_START_ADDR + (i * sizeof(int)), voteCounts[i]);
    
    // Safety check: if EEPROM is uninitialized, reset to 0.
    if(voteCounts[i] < 0 || voteCounts[i] > 1000) { 
      voteCounts[i] = 0;
      EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), voteCounts[i]);
    }
  }
  
  // 4. Enable Watchdog Timer (2 seconds)
  wdt_enable(WDTO_2S);
  
  delay(1000);
  resetToPinEntry();
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  wdt_reset(); // Reset watchdog timer so it knows we haven't frozen
  
  if(currentState == STATE_ENTER_PIN) {
    handlePinEntry();
  } else if (currentState == STATE_VOTING) {
    handleVoting();
  } else if (currentState == STATE_ADMIN) {
    handleAdmin();
  }
}

// ==========================================
// FUNCTIONS
// ==========================================
void resetToPinEntry() {
  currentState = STATE_ENTER_PIN;
  inputPIN = "";
  currentVoterIndex = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Voter PIN:");
}

void handlePinEntry() {
  char key = keypad.getKey();
  
  if (key) {
    // Press * or # to clear mistakes
    if (key == '*' || key == '#') {
       inputPIN = "";
       lcd.setCursor(0, 1);
       lcd.print("                ");
       return;
    }
    
    inputPIN += key;
    lcd.setCursor(0, 1);
    lcd.print(inputPIN);
    
    // Check for Admin Master PIN
    if(inputPIN == adminPIN) {
      currentState = STATE_ADMIN;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ADMIN MODE");
      delay(1000);
      displayResults();
      return;
    }
    
    // When 4 digits are entered, process the login
    if(inputPIN.length() == 4) {
      delay(500); 
      
      // 1. Verify PIN exists in database
      int voterIdx = -1;
      for(int i=0; i<NUM_VOTERS; i++) {
        if(inputPIN == validPINs[i]) {
          voterIdx = i;
          break;
        }
      }
      
      if(voterIdx == -1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid PIN!");
        delay(2000);
        resetToPinEntry();
      } else {
        // 2. Verify they haven't voted yet
        byte hasVoted = EEPROM.read(FLAG_START_ADDR + voterIdx);
        if(hasVoted == 1) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Already Voted!");
          delay(2000);
          resetToPinEntry();
        } else {
          // 3. Access Granted
          currentVoterIndex = voterIdx;
          currentState = STATE_VOTING;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access Granted");
          delay(1000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Select Candidate");
          lcd.setCursor(0, 1);
          lcd.print("1  2  3");
        }
      }
    }
    
    // Error correction limit
    if(inputPIN.length() >= 5) {
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
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Vote Cast!");
        lcd.setCursor(0, 1);
        lcd.print("Thank You.");
        
        lastDebounceTime = millis();
        delay(2000);
        resetToPinEntry();
        return; 
      }
    }
  }
}

void displayResults() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("C1:"); lcd.print(voteCounts[0]);
  lcd.print(" C2:"); lcd.print(voteCounts[1]);
  lcd.setCursor(0, 1);
  lcd.print("C3:"); lcd.print(voteCounts[2]);
  lcd.print(" *:RST");
}

void handleAdmin() {
  char key = keypad.getKey();
  if(key) {
    if(key == '#') {
      resetToPinEntry();
    } else if (key == '*') {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Resetting...");
      
      for(int i=0; i<NUM_CANDIDATES; i++) {
        voteCounts[i] = 0;
        EEPROM.put(VOTE_START_ADDR + (i * sizeof(int)), 0);
      }
      
      for(int i=0; i<NUM_VOTERS; i++) {
        EEPROM.write(FLAG_START_ADDR + i, 0);
      }
      
      delay(1000);
      displayResults();
    }
  }
}
