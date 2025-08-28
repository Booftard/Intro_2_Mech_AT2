#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>

int segPins[8] = {2,3,4,5,6,7,8,9};
int digitPins[4] = {10,11,12,13};

int numberPattern[10][8] = {
    {1,1,1,1,1,1,0,0}, //0
    {0,1,1,0,0,0,0,0}, //1
    {1,1,0,1,1,0,1,0}, //2
    {1,1,1,1,0,0,1,0}, //3
    {0,1,1,0,0,1,1,0}, //4
    {1,0,1,1,0,1,1,0}, //5
    {1,0,1,1,1,1,1,0}, //6
    {1,1,1,0,0,0,0,0}, //7
    {1,1,1,1,1,1,1,0}, //8
    {1,1,1,1,0,1,1,0}  //9
};

int currentDigit = 0; // which digit to update this loop

// --- Countdown Variables

int startSeconds = 0; //This will change to the lockdown count
long countdown = startSeconds * 100;
bool countDownActive = false;
unsigned long zeroTime = 0;

//
const char* ssid = "Booftarded";
const char* password = "Apple@22green";
WiFiUDP udp;
unsigned  int udpPort = 4210;

void setSegments(int digit, bool dp) {
    for (int i = 0; i < 8; i++) {
        int val = numberPattern[digit][i];
        if (i == 7 && dp) val = 1;
        digitalWrite(segPins[i], val);
    }
}

void displayNumber(long num) {
    int cs = num % 100;
    int sec = num / 100;

    int digits[4] = {
        sec / 10, // D1
        sec % 10, // D2
        cs / 10,  // D3
        cs % 10   // D4
    };

    for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], HIGH);

    bool dp = (currentDigit == 1);
    setSegments(digits[currentDigit], dp);

    digitalWrite(digitPins[currentDigit], LOW);

    currentDigit++;
    if (currentDigit > 3) currentDigit = 0;
}

void clearDisplay() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segPins[i], LOW);
    }
    for (int i = 0; i < 4; i++) {
        digitalWrite(digitPins[4], HIGH);
    }
}

void setup() {
    for(int i = 0; i < 8; i++) pinMode(segPins[i], OUTPUT);
    for(int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);

    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Board B Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP()); // FIND BOARD B IP

    udp.begin(udpPort);
}

void loop() {

    // update display as fast as possible
    displayNumber(countdown);
    if (countDownActive) {
        displayNumber(countdown);
    } else {
        clearDisplay();
    }
    int packetSize = udp.parsePacket();
    if(packetSize) {
        char incoming[255];
        int len = udp.read(incoming, 254);
        
        if (len > 0) {
            incoming[len] = '\0';

            if (strcmp(incoming, "LOCK") == 0) {
            countdown = 16 * 100; // CHANGE THIS TO THE TIME NEEDED
            Serial.println("LOCK received! Countdown started.");
            } else if (strcmp(incoming, "UNLOCK") == 0) {
            countdown = 0;
            Serial.println("UNLOCK received! Countdown ");
            } else {
                Serial.print("Unknown message");
                Serial.println(incoming);
            }
        }
    }

    static unsigned long last = 0;
    if (millis() - last >= 10) {
        last = millis();
        if (countdown > 0) {
            countdown--;
        } else if (countDownActive) {
            countDownActive = false;
            if (zeroTime == 0) {
                zeroTime = millis();
            }
            //500ms Delay between when it reaches 0 and when it goes blank
            if (millis() - zeroTime >= 500) {
                countDownActive = false;
                zeroTime = 0;
            }
        }
    }
}

