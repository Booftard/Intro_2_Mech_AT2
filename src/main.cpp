#include <Arduino.h>

// 25342231
const int greenLED = 6;
const int yellowLED = 5;
const int redLED = 4;
const int buttonUnlock = 3;
const int buttonLock = 2;
// SPECIFICATIONS
const int A = 3 * 1000; // solid seconds
const int B = 6;        // Hz / flash interval
const int C = 4;        // flashing seconds
const int D = 5 * 1000; // solid seconds
const int E = 3;        // Hz / flash interval
const int f = 3;        // flashing seconds
const int G = 4;        // Hz / flash interval
const int H = 2;        // flashing seconds
// STATES
enum SystemState {
  UNLOCKED, 
  GREEN_SOLID, 
  GREEN_BLINK, 
  YELLOW_SOLID, 
  YELLOW_BLINK, 
  RED_LOCKED, 
  RED_BLINK
};
// OTHER THINGIES
SystemState currentState = UNLOCKED;
unsigned long lastStateChangeTime = 0;
unsigned long lastBlinkTime = 0;
bool LEDStatus = LOW;
int lockState = 1; // Starts unlocked
// SERIAL REPORT
unsigned long lastReportTime = 0;
const long reportInterval = 1000;
String lastInput = "None";
String activeLED = "Green";
// FOR THE BUTTON MESSAGE SPAM
int lastButtonLockState = LOW;
int lastButtonUnlockState = LOW;

void setup() {
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  pinMode(buttonUnlock, INPUT);
  pinMode(buttonLock, INPUT);

  Serial.begin(9600);
  Serial.println("System initialised");

}

void loop() {
  
  // TEST
  // Serial.print("Current state: ");
  // Serial.println(currentState);

  int lockButtonState = digitalRead(buttonLock);
  int unlockButtonState = digitalRead(buttonUnlock);

  if (unlockButtonState == HIGH && lastButtonUnlockState == LOW && currentState != UNLOCKED) {
    currentState = UNLOCKED;
    lastStateChangeTime = millis();
    lastBlinkTime = millis();
    digitalWrite(greenLED, HIGH);
    digitalWrite(yellowLED, LOW);
    digitalWrite(redLED, LOW);
    lastInput = "Unlock Button";
    Serial.println(">>> UNLOCK BUTTON PRESSED. Starting protocol...");
  } else if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED) {
    currentState = GREEN_SOLID;
    lastStateChangeTime = millis();
    digitalWrite(greenLED, HIGH);
    lastInput = "Lock Button";
    Serial.println(">>> LOCK BUTTON PRESSED. Starting protocol...");
  }

  switch(currentState) {
    case UNLOCKED:
      digitalWrite(greenLED, HIGH);
      digitalWrite(yellowLED, LOW);
      digitalWrite(redLED, LOW);
      activeLED = "Green";
      lockState = 1;
      break;
    case GREEN_SOLID:
      activeLED = "Green";
      if (millis() - lastStateChangeTime >= A) {
        currentState = GREEN_BLINK;
        lastStateChangeTime = millis();
        lastBlinkTime = millis();
      }
      lockState = 2;
      break;
    case GREEN_BLINK:
      activeLED = "Green Blinking";
      if (millis() - lastBlinkTime >= (1000 / B)) {
        LEDStatus = !LEDStatus;
        digitalWrite(greenLED, LEDStatus);
        lastBlinkTime = millis();
      }
      if (millis() - lastStateChangeTime >= C * 1000) {
        digitalWrite(greenLED, LOW);
        currentState = YELLOW_SOLID;
        lastStateChangeTime = millis();
        digitalWrite(yellowLED, HIGH);
      }
      lockState = 3;
      break;
    case YELLOW_SOLID:
      activeLED = "Yellow";
      if (millis() - lastStateChangeTime >= D) {
        currentState = YELLOW_BLINK;
        lastStateChangeTime = millis();
        lastBlinkTime = millis();
      }
      lockState = 4;
      break;
    case YELLOW_BLINK:
      activeLED = "Yellow Blinking";
      if (millis() - lastBlinkTime >= (1000 / E)) {
        LEDStatus = !LEDStatus;
        digitalWrite(yellowLED, LEDStatus);
        lastBlinkTime = millis();
      }
      if (millis() - lastStateChangeTime >= f * 1000) {
        digitalWrite(yellowLED, LOW);
        currentState = RED_LOCKED;
        lastStateChangeTime = millis();
        digitalWrite(redLED, HIGH);
      }
      lockState = 5;
      break;
    case RED_LOCKED:
      digitalWrite(redLED, HIGH);
      digitalWrite(yellowLED, LOW);
      digitalWrite(greenLED, LOW);
      activeLED = "Red";
      lockState = 6;
      break;
    case RED_BLINK:
      activeLED = "Red Blinking";
      if (millis() - lastBlinkTime >= (1000 / G)) {
        LEDStatus = !LEDStatus;
        digitalWrite(redLED, LEDStatus);
        lastBlinkTime = millis();
      }
      if (millis() - lastStateChangeTime >= H * 1000) {
        digitalWrite(redLED, LOW);
        currentState = UNLOCKED;
        digitalWrite(greenLED, HIGH);
      }
      lockState = 7;
      break;
  }

  if (millis() - lastReportTime >= reportInterval) {
    lastReportTime = millis();

    Serial.print("Status: ");
    Serial.print(lockState);

    if (lockState == 1) {
      Serial.print(" Unlocked");
    } else if (lockState == 6) {
      Serial.print(" Locked");
    }

    Serial.print(" | Last Input: ");
    Serial.print(lastInput);
    Serial.print(" | Active LED: ");
    Serial.println(activeLED);
  }

  lastButtonLockState = lockButtonState;
  lastButtonUnlockState = unlockButtonState;
}