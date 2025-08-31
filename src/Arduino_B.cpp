// #include <Arduino.h>
// #include <WiFiS3.h>

// // Segment pins (a, b, c, d, e, f, g, dp)
// int segPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
// // Digit pins (D1, D2, D3, D4)
// int digitPins[4] = {10, 11, 12, 13};

// // Segment patterns for digits 0-9 (0 = segment on, 1 = segment off)
// int numberPattern[10][8] = {
//     {0,0,0,0,0,0,1,1},  // 0
//     {1,0,0,1,1,1,1,1},  // 1
//     {0,0,1,0,0,1,0,1},  // 2
//     {0,0,0,0,1,1,0,1},  // 3
//     {1,0,0,1,1,0,0,1},  // 4
//     {0,1,0,0,1,0,0,1},  // 5
//     {0,1,0,0,0,0,0,1},  // 6
//     {0,0,0,1,1,1,1,1},  // 7
//     {0,0,0,0,0,0,0,1},  // 8
//     {0,0,0,0,1,0,0,1}   // 9
// };

// // Replace with your phone hotspot credentials
// const char* ssid = "Booftarded";
// const char* password = "Apple@22green";

// WiFiServer server(8080);
// int currentDigit = 0;
// int startSeconds = 16;
// long countdown = startSeconds * 10;  // Now in tenths of seconds
// bool countdownActive = false;        // Start inactive
// unsigned long previousMillis = 0;
// unsigned long lastDecrementTime = 0;
// unsigned long countdownCompleteTime = 0;
// const int decrementInterval = 100;   // 100ms for tenths decrement
// const int displayInterval = 2;       // 2ms for display multiplexing
// const int completionDisplayTime = 1000;

// enum WiFiState {
//     WIFI_DISCONNECTED,
//     WIFI_CONNECTING,
//     WIFI_CONNECTED
// };
// WiFiState wifiState = WIFI_DISCONNECTED;
// unsigned long lastWiFiAttempt = 0;
// const unsigned long wifiRetryInterval = 10000;

// void setSegments(int digit, bool dp) {
//     for (int i = 0; i < 8; i++) {
//         int val = numberPattern[digit][i];
//         if (i == 7) {  // DP is the 8th segment (index 7)
//             val = dp ? 0 : 1;  // Invert logic for DP if needed
//         }
//         digitalWrite(segPins[i], val);
//     }
// }

// void clearDisplay() {
//     for (int i = 0; i < 8; i++) {
//         digitalWrite(segPins[i], HIGH);
//     }
//     for (int i = 0; i < 4; i++) {
//         digitalWrite(digitPins[i], LOW);
//     }
// }

// void displayNumber() {
//     // Clear all digits first
//     for (int i = 0; i < 4; i++) {
//         digitalWrite(digitPins[i], LOW);
//     }
    
//     // Clear all segments
//     for (int i = 0; i < 8; i++) {
//         digitalWrite(segPins[i], HIGH);
//     }

//     if (!countdownActive && countdownCompleteTime == 0) {
//         currentDigit = (currentDigit + 1) % 4;
//         return;
//     }

//     if (countdown <= 0 && countdownCompleteTime > 0) {
//         // Display 00.0 when countdown is complete (right-aligned: blank, 0, 0, 0 with DP)
//         if (millis() - countdownCompleteTime < completionDisplayTime) {
//             if (currentDigit == 1) {
//                 setSegments(0, false);
//                 digitalWrite(digitPins[1], HIGH);
//             } else if (currentDigit == 2) {
//                 setSegments(0, true);  // Show decimal point
//                 digitalWrite(digitPins[2], HIGH);
//             } else if (currentDigit == 3) {
//                 setSegments(0, false);
//                 digitalWrite(digitPins[3], HIGH);
//             }
//         } else {
//             countdownCompleteTime = 0;
//             countdownActive = false;
//             countdown = startSeconds * 10;
//         }
        
//     }

//     int totalTenths = countdown;
//     int seconds = totalTenths / 10;
//     int tenths = totalTenths % 10;
    
//     // Determine which digits to display based on the value
//     int tens = seconds / 10;
//     int ones = seconds % 10;
    
//     // Right-align the display - D1 is blank, show time on D2, D3, D4
//     if (currentDigit == 1) {
//         setSegments(tens, false);
//         digitalWrite(digitPins[1], HIGH);
//     } else if (currentDigit == 2) {
//         setSegments(ones, true);  // Show decimal point
//         digitalWrite(digitPins[2], HIGH);
//     } else if (currentDigit == 3) {
//         setSegments(tenths, false);
//         digitalWrite(digitPins[3], HIGH);
//     }
//     // Digit 0 (D1) remains blank
    
//     currentDigit = (currentDigit + 1) % 4;
// }

// void connectToHotspot() {
//   Serial.print("Connecting to hotspot '");
//   Serial.print(ssid);
//   Serial.println("'...");
  
//   WiFi.begin(ssid, password);
  
//   int attempts = 0;
//   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
//     delay(500);
//     attempts++;
//     Serial.print(".");
//   }
  
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("\nConnected to hotspot!");
//     Serial.print("Board B IP: ");
//     Serial.println(WiFi.localIP());
//   } else {
//     Serial.println("\nFailed to connect to hotspot!");
//     Serial.println("Please check:");
//     Serial.println("1. Hotspot is turned ON");
//     Serial.println("2. SSID and password are correct");
//     Serial.println("3. Phone allows Arduino to connect");
//     while(true) { delay(1000); } // Stop here if no connection
//   }
// }

// void startCountdown() {
//   countdown = startSeconds * 10;
//   countdownActive = true;
//   Serial.println("Countdown STARTED - LOCK command received");
// }

// void stopCountdown() {
//   countdownActive = false;
//   countdown = startSeconds * 10; // Reset to initial value
//   Serial.println("Countdown STOPPED/RESET - UNLOCK command received");
// }

// void setup() {
//   for (int i = 0; i < 8; i++) {
//     pinMode(segPins[i], OUTPUT);
//     digitalWrite(segPins[i], HIGH);
//   }
//   for (int i = 0; i < 4; i++) {
//     pinMode(digitPins[i], OUTPUT);
//     digitalWrite(digitPins[i], LOW);
//   }

//   countdown = startSeconds * 10;  // Convert to tenths of seconds
//   countdownActive = false;        // Don't start countdown until LOCK command

//   Serial.begin(9600);
//   delay(1000);
  
//   connectToHotspot();
  
//   // Start server
//   server.begin();
//   Serial.println("Timer Controller Ready");
//   Serial.print("Server IP: ");
//   Serial.println(WiFi.localIP());
//   Serial.println("Waiting for LOCK/UNLOCK commands from Board A...");
// }

// void loop() {
//   unsigned long currentMillis = millis();

//   // Check for incoming commands from Board A
//   WiFiClient client = server.available();
  
//   if (client) {
//     String command = client.readStringUntil('\n');
//     command.trim();
    
//     Serial.print("Received command: ");
//     Serial.println(command);
    
//     if (command == "LOCK") {
//       startCountdown();
//     } 
//     else if (command == "UNLOCK") {
//       stopCountdown();
//     }
    
//     client.stop();
//   }

//   // Display multiplexing
//   if (currentMillis - previousMillis >= displayInterval) {
//     previousMillis = currentMillis;
//     displayNumber();
//   }

//   // Countdown decrement
//   if (countdownActive && currentMillis - lastDecrementTime >= decrementInterval) {
//     lastDecrementTime = currentMillis;
//     if (countdown > 0) {
//       countdown--;
//       if (countdown % 10 == 0) { // Only print full seconds to reduce serial spam
//         Serial.print("Time: ");
//         // Format with leading zero and right alignment for serial monitor
//         if (countdown / 10 < 10) Serial.print("0");
//         Serial.print(countdown / 10);  // Seconds
//         Serial.print(".");
//         Serial.println(countdown % 10);  // Tenths
//       }
//     } else {
//       countdownActive = false;
//       Serial.println("Countdown complete!");
//     }
//   }
// }