
#include <Arduino.h>
#include <WiFiS3.h>

//10.159.167.223 BOARD A IP

// Segment pins (a, b, c, d, e, f, g, dp)
int segPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
// Digit pins (D1, D2, D3, D4)
int digitPins[4] = {10, 11, 12, 13};

// Segment patterns for digits 0-9 (0 = segment on, 1 = segment off)
int numberPattern[10][8] = {
    {0,0,0,0,0,0,1,1},  // 0
    {1,0,0,1,1,1,1,1},  // 1
    {0,0,1,0,0,1,0,1},  // 2
    {0,0,0,0,1,1,0,1},  // 3
    {1,0,0,1,1,0,0,1},  // 4
    {0,1,0,0,1,0,0,1},  // 5
    {0,1,0,0,0,0,0,1},  // 6
    {0,0,0,1,1,1,1,1},  // 7
    {0,0,0,0,0,0,0,1},  // 8
    {0,0,0,0,1,0,0,1}   // 9
};

// Replace with your phone hotspot credentials
const char* ssid = "Booftarded";
const char* password = "Apple@22green";

WiFiServer server(8080);
int currentDigit = 0;   
int startSeconds = 16;
IPAddress boardA_IP(10, 159, 167, 223);
const int boardA_Port = 8080;
WiFiClient client;

long countdown = startSeconds * 10;  // Now in tenths of seconds
bool countdownActive = false;        // Start inactive
unsigned long previousDisplayMillis = 0;
unsigned long previousDecrementMillis = 0;
unsigned long countdownCompleteTime = 0;
const int decrementInterval = 100;   // 100ms for tenths decrement
const int displayInterval = 2;       // 2ms for display multiplexing
const int completionDisplayTime = 1000;

enum WiFiState {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED
};
WiFiState wifiState = WIFI_DISCONNECTED;
unsigned long lastWiFiAttempt = 0;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiRetryInterval = 10000;
const unsigned long wifiCheckInterval = 1000;

enum BootState {
    BOOT_IDLE,
    BOOT_SEGMENTS,
    BOOT_DIGITS,
    BOOT_FLASH,
    BOOT_COMPLETE
};
BootState bootState = BOOT_IDLE;
unsigned long bootStartTime = 0;
int currentTestSegment = 0;
int currentTestDigit = 0;
int flashCount = 0;
bool flashState = false;
bool bootComplete = false;
int currentAnimationSegment = 0;
const int clockwiseSegments[6] = {0,1,2,3,4,5};
unsigned long lastAnimationTime = 0;
const int animationSpeed = 200;

String wifiCommandBuffer = "";
unsigned long lastCommandCheck = 0;
const unsigned long commandCheckInterval = 10;

void setSegments(int digit, bool dp) {
    for (int i = 0; i < 8; i++) {
        int val = numberPattern[digit][i];
        if (i == 7) {  // DP is the 8th segment (index 7)
            val = dp ? 0 : 1;  // Invert logic for DP if needed
        }
        digitalWrite(segPins[i], val);
    }
}

void clearDisplay() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segPins[i], HIGH);
    }
    for (int i = 0; i < 4; i++) {
        digitalWrite(digitPins[i], LOW);
    }
}

void allSegmentsOn() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segPins[i], LOW);
    }
}

void allSegmentsOff() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segPins[i], HIGH);
    }
}

void bootTest() {
    unsigned long currentMillis = millis();

    switch(bootState) {
        case BOOT_IDLE:
            Serial.println(" >>> Starting boot up test");
            bootStartTime = currentMillis;
            bootState = BOOT_SEGMENTS;
            currentTestSegment = 0;
            currentTestDigit = 0;
            break;
        
        case BOOT_SEGMENTS:

        if (currentMillis - bootStartTime >= 500) {
            bootStartTime = currentMillis;

            allSegmentsOff();

            for (int digit = 0; digit < 4; digit++) {
                digitalWrite(digitPins[digit], LOW);
            }

            digitalWrite(segPins[currentTestSegment], LOW);

            for (int digit = 0; digit < 4; digit++) {
                digitalWrite(digitPins[digit], HIGH);
            }

            currentTestSegment++;
            if (currentTestSegment >= 8) {
                bootState = BOOT_DIGITS;
                bootStartTime = currentMillis;
                currentTestDigit = 0;
                Serial.println("Testing all segments per digit");
            }
        }
        break;

        case BOOT_DIGITS:
            if (currentMillis - bootStartTime >= 500) {
                bootStartTime = currentMillis;

                allSegmentsOn();

                for (int digit = 0; digit < 4; digit++) {
                    digitalWrite(digitPins[digit], LOW);
                }

                digitalWrite(digitPins[currentTestDigit], HIGH);

                currentTestDigit++;
                if (currentTestDigit >= 4) {
                    bootState = BOOT_FLASH;
                    bootStartTime = currentMillis;
                    flashCount = 0;
                    flashState = false;
                    Serial.println("Final flash test");
                }
            }
            break;
        case BOOT_FLASH:
            if (currentMillis - bootStartTime >= 200) {
                bootStartTime = currentMillis;

                if (flashState) {
                    for (int digit = 0; digit < 4; digit++) {
                        digitalWrite(digitPins[digit], HIGH);
                    }
                    
                    allSegmentsOn();
                } else {
                    for (int digit = 0; digit < 4; digit++) {
                        digitalWrite(digitPins[digit], LOW);
                    }
                    allSegmentsOff();
                }

                flashState = !flashState;
                if (!flashState) {
                    flashCount++;
                    if (flashCount >= 3) {
                        bootState = BOOT_COMPLETE;
                        clearDisplay();
                        Serial.println(">>> BOOT TEST COMPLETE <<<");
                        bootComplete = true;
                        wifiState = WIFI_DISCONNECTED;
                    }
                }
            }
            break;
        case BOOT_COMPLETE:
            break;
    }
}

void connectionAnimation() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastAnimationTime >= animationSpeed) {
        lastAnimationTime = currentMillis;

        clearDisplay();

        for (int seg = 0; seg < 8; seg++) {
            if (seg != 6 && seg != 7) {
                digitalWrite(segPins[seg], LOW);
            }
        }

        digitalWrite(segPins[clockwiseSegments[currentAnimationSegment]], HIGH);

        for (int digit = 0; digit < 4; digit++) {
            digitalWrite(digitPins[digit], HIGH);
        }

        currentAnimationSegment = (currentAnimationSegment + 1) % 6;
    }
}

void sendMessageToBoardA(const char* command) {
    if (wifiState == WIFI_CONNECTED && client.connect(boardA_IP, boardA_IP)) {
        client.println(command);
        client.stop();
    }
}

void displayNumber() {
    // Clear all digits first
    for (int i = 0; i < 4; i++) {
        digitalWrite(digitPins[i], LOW);
    }
    
    // Clear all segments
    for (int i = 0; i < 8; i++) {
        digitalWrite(segPins[i], HIGH);
    }

    if (!countdownActive && countdownCompleteTime == 0) {
        currentDigit = (currentDigit + 1) % 4;
        return;
    }

    if (countdown <= 0 && countdownCompleteTime > 0) {
        // Display 00.0 when countdown is complete (right-aligned: blank, 0, 0, 0 with DP)
        if (millis() - countdownCompleteTime < completionDisplayTime) {
            if (currentDigit == 1) {
                setSegments(0, false);
                digitalWrite(digitPins[1], HIGH);
            } else if (currentDigit == 2) {
                setSegments(0, true);  // Show decimal point
                digitalWrite(digitPins[2], HIGH);
            } else if (currentDigit == 3) {
                setSegments(0, false);
                digitalWrite(digitPins[3], HIGH);
            }
        } else {
            countdownCompleteTime = 0;
            countdownActive = false;
            countdown = startSeconds * 10;
        }
        currentDigit = (currentDigit + 1) % 4;
        return;
    }

    int totalTenths = countdown;
    int seconds = totalTenths / 10;
    int tenths = totalTenths % 10;
    
    // Determine which digits to display based on the value
    int tens = seconds / 10;
    int ones = seconds % 10;
    
    // Right-align the display - D1 is blank, show time on D2, D3, D4
    if (currentDigit == 1) {
        setSegments(tens, false);
        digitalWrite(digitPins[1], HIGH);
    } else if (currentDigit == 2) {
        setSegments(ones, true);  // Show decimal point
        digitalWrite(digitPins[2], HIGH);
    } else if (currentDigit == 3) {
        setSegments(tenths, false);
        digitalWrite(digitPins[3], HIGH);
    }
    // Digit 0 (D1) remains blank
    
    currentDigit = (currentDigit + 1) % 4;
}

void connectToHotspot() {
  unsigned long currentMillis = millis();

  switch(wifiState) {
    case WIFI_DISCONNECTED:
        if (currentMillis - lastWiFiAttempt >= wifiRetryInterval) {
            Serial.println("Starting Wifi Connection");
            WiFi.begin(ssid, password);
            wifiState = WIFI_CONNECTING;
            lastWiFiAttempt = currentMillis;
        }
        break;
    case WIFI_CONNECTING:
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = WIFI_CONNECTED;
            Serial.println("Connected to hotspot");
            Serial.print("Board B IP: ");
            Serial.println(WiFi.localIP());

            server.begin();
            Serial.println("Timer Controller ready");
            Serial.println("Waiting for LOCK/UNLOCK message from board A");
        } else if (currentMillis - lastWiFiAttempt > 15000) {
            Serial.println("WiFi connection failed, retrying ...");
            wifiState = WIFI_DISCONNECTED;
            WiFi.disconnect();
            lastWiFiAttempt = currentMillis;
        }
        break;
    case WIFI_CONNECTED:
        if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
            lastWiFiCheck = currentMillis;
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi connection lost");
                wifiState = WIFI_DISCONNECTED;
                lastWiFiAttempt = currentMillis;
            }
        }
        break;
  }
}

void startCountdown() {
  countdown = startSeconds * 10;
  countdownActive = true;
  countdownCompleteTime = 0;
  Serial.println("Countdown STARTED - LOCK command received");
}

void stopCountdown() {
  countdownActive = false;
  countdownCompleteTime = 0;
  countdown = startSeconds * 10; // Reset to initial value
  Serial.println("Countdown STOPPED/RESET - UNLOCK command received");
}

void checkForCommands() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastCommandCheck >= commandCheckInterval) {
        lastCommandCheck = currentMillis;

        WiFiClient client = server.available();
        if (client) {
            while (client.connected() && client.available()) {
                char c = client.read();
                if (c == '\n') {
                    wifiCommandBuffer.trim();
                    if (wifiCommandBuffer.length() > 0) {
                        Serial.print("Received command: ");
                        Serial.println(wifiCommandBuffer);

                        if (wifiCommandBuffer == "LOCK") {
                            startCountdown();
                        } else if (wifiCommandBuffer == "UNLOCK") {
                            stopCountdown();
                        }
                        wifiCommandBuffer = "";
                    }
                } else if (c != '\r') {
                    wifiCommandBuffer += c;
                    if (wifiCommandBuffer.length() > 20) {
                        wifiCommandBuffer = "";
                    }
                }
            }
            client.stop();
        }
    }
}

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], HIGH);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], LOW);
  }

  countdown = startSeconds * 10;  // Convert to tenths of seconds
  countdownActive = false;        // Don't start countdown until LOCK command
  countdownCompleteTime = 0;

  Serial.begin(9600);
  delay(1000);
  
  clearDisplay();
  
  bootState = BOOT_IDLE;
  bootComplete = false;
}

void loop() {
  unsigned long currentMillis = millis();

  if (!bootComplete) {
    bootTest();
    return;
  }

  if (currentMillis - previousDisplayMillis >= displayInterval) {
    previousDisplayMillis = currentMillis;
    displayNumber();
  }

  connectToHotspot();

  if (wifiState == WIFI_CONNECTED) {
    checkForCommands();
  } else {
    connectionAnimation();
  }

  // Countdown decrement
  if (countdownActive && wifiState == WIFI_CONNECTED && countdownActive 
    && currentMillis - previousDecrementMillis >= decrementInterval) {

    previousDecrementMillis = currentMillis;
    if (countdown > 0) {
      countdown--;
      if (countdown % 10 == 0) { // Only print full seconds to reduce serial spam
        Serial.print("Time: ");
        // Format with leading zero and right alignment for serial monitor
        if (countdown / 10 < 10) Serial.print("0");
        Serial.print(countdown / 10);  // Seconds
        Serial.print(".");
        Serial.println(countdown % 10);  // Tenths
      }
    } else {
      countdownActive = false;
      countdownCompleteTime = millis();
      Serial.println("Countdown complete!");
    }
  }
}