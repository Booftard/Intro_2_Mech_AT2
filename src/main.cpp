#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

// 25342231
const int greenLED = 6;
const int yellowLED = 5;
const int redLED = 4;
const int buttonUnlock = 3;
const int buttonLock = 2;
// SPECIFICATIONS
const int A = 3 * 1000;        // solid seconds
const int B = 6;               // Hz / flash interval
const int C = 4 * 1000;        // flashing seconds
const int D = 5 * 1000;        // solid seconds
const int E = 3;               // Hz / flash interval
const int f = 3 * 1000;        // flashing seconds
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
int lockState = 1; // Starts unlocked
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
unsigned long lastSend = 0;
const unsigned long sendInterval = 1000; //1 second between test messages

void sendMessageToBoardB(const char* msg) {
  udp.beginPacket(BoardB_IP, udpPort);
  udp.write(msg);
  udp.endPacket();
  Serial.print("Sent to Board B: ");
  Serial.println(msg);
}

void setSolidLed(int pin) {
  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(redLED, LOW);

  digitalWrite(pin, HIGH);

  LEDStatus = LOW;
  lastBlinkTime = millis();
}

void blinkLed(int pin, unsigned long frequencyHz) {
  unsigned long now = millis();
  unsigned long frequencyMs = 1000.0 / frequencyHz;

  if (now - lastBlinkTime >= frequencyMs) {
    lastBlinkTime = now;
    LEDStatus = !LEDStatus;
  }

  digitalWrite(greenLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(redLED, LOW);

  digitalWrite(pin, LEDStatus ? HIGH : LOW);
}

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
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  pinMode(buttonUnlock, INPUT);
  pinMode(buttonLock, INPUT);
  
  Serial.begin(115200);
  Serial.println("System initialised");

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
  
  // TEST
  // Serial.print("Current state: ");
  // Serial.println(currentState);

  int lockButtonState = digitalRead(buttonLock);
  int unlockButtonState = digitalRead(buttonUnlock);
  activeLED = getLEDName(currentState);

  if (unlockButtonState == HIGH && lastButtonUnlockState == LOW) {
    
    lastStateChangeTime = millis();
    lastBlinkTime = millis();
    lastInput = "Unlock Button";
    
    if (currentState == RED_LOCKED) {
      currentState = RED_BLINK;
    } else if (currentState != UNLOCKED) {
      currentState = UNLOCKED;
      setSolidLed(greenLED);
    }
    Serial.println(">>> UNLOCK BUTTON PRESSED. Starting protocol...");
    sendMessageToBoardB("UNLOCK");
  } else if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED) {
    
    currentState = GREEN_SOLID;
    lastStateChangeTime = millis();
    lastInput = "Lock Button";
    setSolidLed(greenLED);
    
    Serial.println(">>> LOCK BUTTON PRESSED. Starting protocol...");
    sendMessageToBoardB("LOCK");
  }

  switch(currentState) {
    case UNLOCKED:
      setSolidLed(greenLED);
      lockState = 1;
      break;
    case GREEN_SOLID:
      setSolidLed(greenLED);
      if (millis() - lastStateChangeTime >= A) {
        currentState = GREEN_BLINK;
        lastStateChangeTime = millis();
      }
      break;
    case GREEN_BLINK:
      blinkLed(greenLED, B);
      if (millis() - lastStateChangeTime >= C) {
        currentState = YELLOW_SOLID;
        lastStateChangeTime = millis();
      }
      break;
    case YELLOW_SOLID:
      setSolidLed(yellowLED);
      if (millis() - lastStateChangeTime >= D) {
        currentState = YELLOW_BLINK;
        lastStateChangeTime = millis();
      }
      break;
    case YELLOW_BLINK:
      blinkLed(yellowLED, E);
      if (millis() - lastStateChangeTime >= f) {
        currentState = RED_LOCKED;
        lastStateChangeTime = millis();
      }
      break;
    case RED_LOCKED:
      setSolidLed(redLED);
      lockState = 6;
      break;
    case RED_BLINK:
      blinkLed(redLED, G);
      if (millis() - lastStateChangeTime >= H) {
        currentState = UNLOCKED;
        lastStateChangeTime = millis();
      }
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

