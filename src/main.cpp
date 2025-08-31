// #include <Arduino.h>
// #include <WiFiS3.h>
// //10.159.167.38 BOARD B IP

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

// //  WIFI STATES
// enum WiFiStates {
//     WIFI_DISCONNECTED,
//     WIFI_CONNECTING,
//     WIFI_CONNECTED,
//     WIFI_FAILED
// };

// // WIFI STUFF
// const char* ssid = "Booftarded";
// const char * password = "Apple@22green";

// IPAddress boardB_IP(10, 159, 167, 38);
// const int boardB_Port = 8080;

// WiFiClient client;

// WiFiStates wifiState = WIFI_DISCONNECTED;
// bool systemReady = false;
// unsigned long lastWiFiAttempt = 0;
// const unsigned long wifiRetryInterval = 10000;

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

//   void sendMessageToBoardB(const char* command) {
//     if(wifiState == WIFI_CONNECTED && client.connect(boardB_IP, boardB_Port)) {
//         client.println(command);
//         client.stop();
//         if (authenticated) {
//             Serial.print("Sent to board B: ");
//             Serial.println(command);
//         }
//     }
//   }

//   void connectToWiFi() {
//     //HAVE LIGHT BLUE FOR WIFI BECAUSE I FEEL LIKE IT
//     unsigned long currentMillis = millis();

//     switch(wifiState) {
//         case WIFI_DISCONNECTED:
//             WiFi.begin(ssid, password);
//             wifiState = WIFI_CONNECTING;
//             lastWiFiAttempt = currentMillis;
//             if (authenticated) {
//                 Serial.println("Starting WiFi connection...");
//             }
//             break;
//         case WIFI_CONNECTING:
//             if (WiFi.status() == WL_CONNECTED) {
//                 wifiState = WIFI_CONNECTED;
//                 systemReady = true;
//                 if (authenticated) {
//                     Serial.println("WiFi connected");

//                 }
//                 setSolidRGB(LOW, HIGH, LOW);
//                 currentState = UNLOCKED;
//             } else if (currentMillis - lastWiFiAttempt > 15000) {
//                 wifiState = WIFI_FAILED;
//                 if (authenticated) {
//                     Serial.println("WiFi connection failed");
//                 }
//             }
//             break;
//         case WIFI_FAILED:
//             if (currentMillis - lastWiFiAttempt > wifiRetryInterval) {
//                 wifiState = WIFI_DISCONNECTED;
//             }
//             break;
//         case WIFI_CONNECTED:
//             if (WiFi.status() != WL_CONNECTED) {
//                 wifiState = WIFI_DISCONNECTED;
//                 systemReady = false;
//                 if (authenticated) {
//                     Serial.println("WiFi connection lost");
//                 }
//             }
//             break;
//     }

//     switch(wifiState) {
//         case WIFI_DISCONNECTED:
//         case WIFI_FAILED:
//             setSolidRGB(LOW, LOW, HIGH);
//             break;
//         case WIFI_CONNECTING:
//             if (currentMillis % 1000 < 500) {
//                 setSolidRGB(LOW, LOW, HIGH);
//             } else {
//                 setSolidRGB(LOW, LOW, 100);
//             }
//             break;
//         case WIFI_CONNECTED:
//             break;
//     }
//   }

//   void setup() {
//     pinMode(redPin, OUTPUT);
//     pinMode(greenPin, OUTPUT);
//     pinMode(bluePin, OUTPUT);

//     pinMode(buttonUnlock, INPUT);
//     pinMode(buttonLock, INPUT);

//     Serial.begin(9600);
    
//     setSolidRGB(LOW, LOW, HIGH);
//     wifiState = WIFI_DISCONNECTED;
//   }

//   void loop() {

//     connectToWiFi();

//     if (!systemReady || wifiState != WIFI_CONNECTED) {
//         return;
//     }

//     int stateNumber = (int)currentState;
    
//     int lockButtonState = digitalRead(buttonLock);
//     int unlockButtonState = digitalRead(buttonUnlock);
//     activeLED = getLEDName(currentState);

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

//     if (unlockButtonState == HIGH && lastButtonUnlockState == LOW && currentState != UNLOCKED && isLastButtonLock) {
//       lastStateChangeTime = millis();
//       lastBlinkTime = millis();
//       lastInput = "Unlock";

//       if (currentState == RED_LOCKED) {
//         currentState = RED_BLINK;
//       } else {
//         currentState = UNLOCKED;
//       }

//       sendMessageToBoardB("UNLOCK");

//       if (authenticated) {
//         Serial.println(">>> UNLOCK BUTTON PRESSED. STARTING PROTOCOL...");
//       }
//       isLastButtonLock = false;
//     } 

//     if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED && !isLastButtonLock) {
      
//       currentState = GREEN_SOLID;
//       lastStateChangeTime = millis();
//       lastInput = "Lock";

//       sendMessageToBoardB("LOCK");

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
//       Serial.print(analogRead(potPin));
//       Serial.print("    | WiFi: ");
//       Serial.println(wifiState == WIFI_CONNECTED ? "Connected" : "Disconnected");
//     }

//     lastButtonLockState = lockButtonState;
//     lastButtonUnlockState = unlockButtonState;
//   }