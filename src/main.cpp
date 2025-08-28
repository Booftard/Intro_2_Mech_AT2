//LOOK AT BOTTOM TO ADD
#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

// 25342231
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int buttonUnlock = 4;
const int buttonLock = 2;
const int potPin = A0;

// SPECIFICATIONS
const int A = 3 * 1000;        // solid seconds
const int B = 6;               // Hz / flash interval
const int C = 4 * 1000;        // flashing seconds
const int D = 5 * 1000;        // solid seconds
const int E = 3;               // Hz / flash interval
const int f = 4 * 1000;        // flashing seconds
const int G = 4;               // Hz / flash interval
const int H = 2 * 1000;        // flashing seconds
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
int potValue = analogRead(potPin);
// SERIAL REPORT
unsigned long lastReportTime = 0;
const long reportInterval = 1000;
String lastInput = "None";
String activeLED = "Green";
// FOR THE BUTTON MESSAGE SPAM
int lastButtonLockState = LOW;
int lastButtonUnlockState = LOW;
// UDP SetUP
const char* ssid = "Booftarded";   //NAME OF HOTSPOT
const char* password = "Apple@22green"; //min of 8 char
WiFiUDP udp;
unsigned int udpPort = 4210;
IPAddress BoardB_IP(192,168,43,100);

// PASSWORD STUFF
const char* password = "password";
bool authenticated = false;
String lastAttempt = "";

void sendMessageToBoardB(const char* msg) {
  udp.beginPacket(BoardB_IP, udpPort);
  udp.write(msg);
  udp.endPacket();
  Serial.print("Sent to Board B: ");//remove this once we know it works
  Serial.println(msg);
}
//The ? in analogWrite is a terniary operator (if value is true -> brightness else value is false -> 0)
void setSolidRGB(int red, int green, int blue) {
  int potValue = analogRead(potPin);
  int brightness = map(potValue, 0, 1023, 0, 255);

  analogWrite(redPin, red == HIGH ? brightness : 0);
  analogWrite(greenPin, green == HIGH ? brightness : 0);
  analogWrite(bluePin, blue == HIGH ? brightness : 0);

  LEDStatus = LOW;
  lastBlinkTime = millis();
}

void blinkRGB(int red, int green, int blue, unsigned long frequencyHz) {
  unsigned long now = millis();
  unsigned long frequencyMs = 1000.0 / frequencyHz;

  if (now - lastBlinkTime >= frequencyMs) {
    lastBlinkTime = now;
    LEDStatus = !LEDStatus;
  }
  
  int potValue = analogRead(potPin);
  int brightness = map(potValue, 0, 1023, 0, 255);

  if (LEDStatus) {
    analogWrite(redPin, red == HIGH ? brightness : 0);
    analogWrite(greenPin, green == HIGH ? brightness : 0);
    analogWrite(bluePin, blue == HIGH ? brightness : 0);
  } else {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
  }
}
//Remove the "blinking" from the blink cases once we know it works
String getLEDName(SystemState s) {
  switch(s) {
    case UNLOCKED: return "Green";
    case GREEN_SOLID: return "Green";
    case GREEN_BLINK: return "Green Blinking";
    case YELLOW_SOLID: return "Yellow";
    case YELLOW_BLINK: return "Yellow Blinking";
    case RED_LOCKED: return "Red";
    case RED_BLINK: return "Red Blinking";
  }
  return "Unknown";
}

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  pinMode(buttonUnlock, INPUT);
  pinMode(buttonLock, INPUT);
  
  Serial.begin(115200);

  Serial.println("Enter password to enable serial monitor: ");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Board A connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(udpPort);
}

void loop() {
  
  //The check for password
  if (!authenticated && Serial.available() > 0) {
    char incomingChar = Serial.read();
    if (incomingChar == '\n' || incomingChar == '\r') {
      lastAttempt.trim();
      if (lastAttempt.equals(password)) {
        Serial.println("Password accepted have fun");
        authenticated = true;
      } else {
        Serial.println("Password incorrect please try again");
      }
      lastAttempt = "";
    } else {
      password += incomingChar;
    }
  } 

  int stateNumber = (int)currentState;

  int lockButtonState = digitalRead(buttonLock);
  int unlockButtonState = digitalRead(buttonUnlock);
  activeLED = getLEDName(currentState);

  if (unlockButtonState == HIGH && lastButtonUnlockState == LOW) {
    
    lastStateChangeTime = millis();
    lastBlinkTime = millis();
    lastInput = "Unlock";
    
    if (currentState == RED_LOCKED) {
      currentState = RED_BLINK;
    } else if (currentState != UNLOCKED) {
      currentState = UNLOCKED;
      setSolidRGB(LOW, HIGH, LOW);
    }
    if (authenticated) {
      Serial.println(">>> UNLOCK BUTTON PRESSED. Starting protocol...");
    }
    sendMessageToBoardB("UNLOCK");
  } else if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED) {
    
    currentState = GREEN_SOLID;
    lastStateChangeTime = millis();
    lastInput = "Lock";
    setSolidRGB(LOW, HIGH, LOW);
    
    if (authenticated) {
      Serial.println(">>> LOCK BUTTON PRESSED. Starting protocol...");
    }
    sendMessageToBoardB("LOCK");
  }

  switch(currentState) {
    case UNLOCKED:
      setSolidRGB(LOW, HIGH, LOW);
      break;
    case GREEN_SOLID:
      setSolidRGB(LOW, HIGH, LOW);
      if (millis() - lastStateChangeTime >= A) {
        currentState = GREEN_BLINK;
        lastStateChangeTime = millis();
      }
      break;
    case GREEN_BLINK:
      blinkRGB(LOW, HIGH, LOW, B);
      if (millis() - lastStateChangeTime >= C) {
        currentState = YELLOW_SOLID;
        lastStateChangeTime = millis();
      }
      break;
    case YELLOW_SOLID:
      setSolidRGB(HIGH, HIGH, LOW);
      if (millis() - lastStateChangeTime >= D) {
        currentState = YELLOW_BLINK;
        lastStateChangeTime = millis();
      }
      break;
    case YELLOW_BLINK:
      blinkRGB(HIGH, HIGH, LOW, E);
      if (millis() - lastStateChangeTime >= f) {
        currentState = RED_LOCKED;
        lastStateChangeTime = millis();
      }
      break;
    case RED_LOCKED:
      setSolidRGB(HIGH, LOW, LOW);
      break;
    case RED_BLINK:
      blinkRGB(HIGH, LOW, LOW, G);
      if (millis() - lastStateChangeTime >= H) {
        currentState = UNLOCKED;
        lastStateChangeTime = millis();
      }
      break;
  }

  if (authenticated && millis() - lastReportTime >= reportInterval) {
    lastReportTime = millis();

    Serial.print("State: ");
    Serial.print(stateNumber);

    if (stateNumber != (int)RED_LOCKED && stateNumber != (int)RED_BLINK) {
      Serial.print(" | Unlocked");
    } else {
      Serial.print(" | Locked");
    }

    Serial.print("    | Last Button: ");
    Serial.print(lastInput);
    Serial.print("    | Status Light: ");
    Serial.print(activeLED);
    Serial.print("    | Reading: ");
    Serial.print(potPin);
  }

  lastButtonLockState = lockButtonState;
  lastButtonUnlockState = unlockButtonState;
}

