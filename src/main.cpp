// #include <Arduino.h>

// //PINS IN USE
// const int redPin = 3;
// const int greenPin = 5;
// const int bluePin = 6;
// const int buttonUnlock = 4;
// const int buttonLock = 2;
// const int potPin = A0;

// // SPECIFICATIONS
// const int A = 3 * 1000;     // solid seconds
// const int B = 6;            // Hz / flash interval
// const int C = 4 * 1000;     // flashing seconds
// const int D = 5 * 1000;     // solid seconds
// const int E = 3;            // Hz / flash interval
// const int f = 4 * 1000;     // flashing seconds
// const int G = 4;            // Hz / flash interval
// const int H = 2 * 1000;     // flashing seconds

// // STATES
// enum SystemState {
//   UNLOCKED,
//   GREEN_SOLID,
//   GREEN_BLINK,
//   YELLOW_SOLID,
//   YELLOW_BLINK,
//   RED_LOCKED,
//   RED_BLINK
// };

// // MISS-EL-AIN-EOUS
// SystemState currentState = UNLOCKED;
// unsigned long lastStateChangeTime = 0;
// unsigned long lastBlinkTime = 0;
// bool LEDStatus = LOW;

// // SERIAL REPORTING 
// unsigned long lastReportTime = 0;
// const long reportInterval = 1000;
// String lastInput = "None";
// String activeLED = "Green";

// // BUTTON SPAM PREVENTOR
// int lastButtonLockState = LOW;
// int lastButtonUnlockState = LOW;
// bool isLastButtonLock = false;

// // PASSWORD STUFF
// const String masterPassword = "password";
// bool authenticated = false;
// String lastPassAttempt = "";

// void setSolidRGB(int red, int green, int blue) {
//   int potValue = analogRead(potPin);
//   int brightness = map(potValue, 0, 1023, 0, 255);

//   analogWrite(redPin, red == HIGH ? brightness : 0);
//   analogWrite(greenPin, green == HIGH ? brightness : 0);
//   analogWrite(bluePin, blue == HIGH ? brightness : 0);

//   LEDStatus = LOW;
//   lastBlinkTime = millis();
// }

// void setBlinkRGB(int red, int green, int blue, unsigned long frequencyHz) {
//   unsigned long now = millis();
//   unsigned long frequencyMs = 1000.0 / frequencyHz;

//   if (now - lastBlinkTime >= frequencyMs) {
//     lastBlinkTime = now;
//     LEDStatus = !LEDStatus;
//   }

//   int potValue = analogRead(potPin);
//   int brightness = map(potValue, 0, 1023, 0, 255);

//   if (LEDStatus) {
//     analogWrite(redPin, red == HIGH ? brightness : 0);
//     analogWrite(greenPin, green == HIGH ? brightness : 0);
//     analogWrite(bluePin, blue == HIGH ? brightness : 0);
//   } else {
//     analogWrite(redPin, 0);
//     analogWrite(greenPin, 0);
//     analogWrite(bluePin, 0);
//   }
// }

// String getLEDName(SystemState s) {
//     switch(s) {
//       case UNLOCKED: return "Green Unlocked";
//       case GREEN_SOLID: return "Green Solid";
//       case GREEN_BLINK: return "Green Blink";
//       case YELLOW_SOLID: return "Yellow Solid";
//       case YELLOW_BLINK: return "Yellow Blink";
//       case RED_LOCKED: return "Red Locked";
//       case RED_BLINK: return "Red Blink";
//     }
//     return "Unknown";
//   }

//   void setup() {
//     pinMode(redPin, OUTPUT);
//     pinMode(greenPin, OUTPUT);
//     pinMode(bluePin, OUTPUT);

//     pinMode(buttonUnlock, INPUT);
//     pinMode(buttonLock, INPUT);

//     Serial.begin(9600);
//     delay(1000);
//     Serial.println("Enter password to enable serial monitor: ");
//   }

//   void loop() {

//     if (!authenticated && Serial.available() > 0) {
//       String incomingStr = Serial.readStringUntil('\n');
//       incomingStr.trim();

//       if (incomingStr.equals(masterPassword)) {
//         Serial.println("Password accepted have fun!");
//         authenticated = true;
//       } else {
//         Serial.println("Password incorrect please try again");
//       }
//     }
    
//     int stateNumber = (int)currentState;
    
//     int lockButtonState = digitalRead(buttonLock);
//     int unlockButtonState = digitalRead(buttonUnlock);
//     activeLED = getLEDName(currentState);

//     if (unlockButtonState == HIGH && lastButtonUnlockState == LOW && currentState != UNLOCKED && isLastButtonLock) {
//       lastStateChangeTime = millis();
//       lastBlinkTime = millis();
//       lastInput = "Unlock";

//       if (currentState == RED_LOCKED) {
//         currentState = RED_BLINK;
//       } else {
//         currentState = UNLOCKED;
//       }
//       if (authenticated) {
//         Serial.println(">>> UNLOCK BUTTON PRESSED. STARTING PROTOCOL...");
//       }
//       isLastButtonLock = false;
//     } 

//     if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED && !isLastButtonLock) {
      
//       currentState = GREEN_SOLID;
//       lastStateChangeTime = millis();
//       lastInput = "Lock";

//       if (authenticated) {
//         Serial.println(">>> LOCK BUTTON PRESSED. STARTING PROTOCOL...");
//       }
//       isLastButtonLock = true;
//     }

//     switch(currentState) {
//       case UNLOCKED:
//         setSolidRGB(LOW, HIGH, LOW);
//         break;
//       case GREEN_SOLID:
//         setSolidRGB(LOW, HIGH, LOW);
//         if (millis() - lastStateChangeTime >= A) {
//           currentState = GREEN_BLINK;
//           lastStateChangeTime = millis();
//         }
//         break;
//       case GREEN_BLINK:
//         setBlinkRGB(LOW, HIGH, LOW, B);
//         if (millis() - lastStateChangeTime >= C) {
//           currentState = YELLOW_SOLID;
//           lastStateChangeTime = millis();
//         }
//         break;
//       case YELLOW_SOLID:
//         setSolidRGB(HIGH, HIGH, LOW);
//         if (millis() - lastStateChangeTime >= D) {
//           currentState = YELLOW_BLINK;
//           lastStateChangeTime = millis();
//         }
//         break;
//       case YELLOW_BLINK:
//         setBlinkRGB(HIGH, HIGH, LOW, E);
//         if (millis() - lastStateChangeTime >= f) {
//           currentState = RED_LOCKED;
//           lastStateChangeTime = millis();
//         }
//         break;
//       case RED_LOCKED:
//         setSolidRGB(HIGH, LOW, LOW);
//           break;
//       case RED_BLINK:
//         setBlinkRGB(HIGH, LOW, LOW, G);
//         if (millis() - lastStateChangeTime >= H) {
//           currentState = UNLOCKED;
//           lastStateChangeTime = millis();
//         }
//         break;
//     }

//     if (authenticated && millis() - lastReportTime >= reportInterval) {
//       lastReportTime = millis();

//       Serial.print("State: ");
//       Serial.print(stateNumber + 1);//starts from 0 so add 1 

//       if (stateNumber != (int)RED_LOCKED && stateNumber != (int)RED_BLINK) {
//         Serial.print(" | Unlocked");
//       } else {
//         Serial.print(" | Locked");
//       }

//       Serial.print("   | Last Button: ");
//       Serial.print(lastInput);
//       Serial.print("   | Status Light: ");
//       Serial.print(activeLED);
//       Serial.print("   | Reading: ");
//       Serial.println(analogRead(potPin));
//     }

//     lastButtonLockState = lockButtonState;
//     lastButtonUnlockState = unlockButtonState;
//   }