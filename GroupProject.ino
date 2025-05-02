#include <Keypad.h>
#include <LiquidCrystal.h>

// LCD Setup
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Password Logic
String password = "1234";
String input = "";
int failedAttempts = 0;
const int maxAttempts = 3;

// Pins
const int buzzerPin = 8;
const int greenLEDPin = 10;
const int redLEDPin = 11;

// Face recognition flags
bool faceScanned = false;
bool faceMatched = false;
bool waitingForPIN = false;

void setup() {
  lcd.begin(16, 2);
  lcd.print("Initializing...");

  pinMode(buzzerPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(redLEDPin, LOW);

  Serial.begin(9600);
  while (!Serial);

  lcd.clear();
  lcd.print("Scan Your Face");
  Serial.println("SCAN_FACE");
}

void loop() {
  // Wait for face scan result
  if (!faceScanned) {
    String response = "";
    unsigned long timeout = millis() + 10000;

    while (millis() < timeout) {
      if (Serial.available()) {
        response = Serial.readStringUntil('\n');
        response.trim(); // Removes newline/whitespace

        Serial.print("Received from Python: ");
        Serial.println(response);

        if (response == "MATCH") {
          faceMatched = true;
        } else if (response == "NO_MATCH") {
          faceMatched = false;
        }
        faceScanned = true;
        break;
      }
    }

    lcd.clear();
    if (faceMatched) {
      lcd.print("Access Granted");
      digitalWrite(greenLEDPin, HIGH);
      delay(2000);
      digitalWrite(greenLEDPin, LOW);
      resetState();
      return;
    } else {
      lcd.print("Face Not Found");
      delay(1000);
      lcd.clear();
      lcd.print("Enter Password:");
      waitingForPIN = true;
    }
  }

  // Keypad logic if face not matched
  if (waitingForPIN) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') {
        lcd.clear();
        if (input == password) {
          lcd.print("Access Granted");
          digitalWrite(greenLEDPin, HIGH);
          delay(1000);
          digitalWrite(greenLEDPin, LOW);
          resetState();
        } else {
          failedAttempts++;
          lcd.print("Wrong Password");
          blinkRedLED();
          delay(800);
          input = "";

          if (failedAttempts >= maxAttempts) {
            lcd.clear();
            lcd.print("Locked Out!");
            digitalWrite(redLEDPin, HIGH);
            digitalWrite(buzzerPin, HIGH);
            delay(1000);
            digitalWrite(buzzerPin, LOW);

            for (int i = 5; i > 0; i--) {
              lcd.setCursor(0, 1);
              lcd.print("Retry in: ");
              lcd.print(i);
              lcd.print("s   ");
              delay(1000);
            }

            digitalWrite(redLEDPin, LOW);
            failedAttempts = 0;
            input = "";
            waitingForPIN = false;
            faceScanned = false;
            lcd.clear();
            lcd.print("Scan Your Face");
            Serial.println("SCAN_FACE");
          } else {
            lcd.clear();
            lcd.print("Enter Password:");
          }
        }
      } else if (key == '*') {
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
      } else {
        input += key;
        lcd.setCursor(0, 1);
        lcd.print(input);
      }
    }
  }

  // Optional: handle Python sending EXIT
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "EXIT") {
      digitalWrite(greenLEDPin, LOW);
      digitalWrite(redLEDPin, LOW);
      digitalWrite(buzzerPin, LOW);
    }
  }
}

void blinkRedLED() {
  digitalWrite(redLEDPin, HIGH);
  delay(200);
  digitalWrite(redLEDPin, LOW);
}

void resetState() {
  failedAttempts = 0;
  input = "";
  faceMatched = false;
  faceScanned = false;
  waitingForPIN = false;
  lcd.clear();
  lcd.print("Scan Your Face");
  Serial.println("SCAN_FACE");
}
