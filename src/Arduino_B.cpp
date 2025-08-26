#include <Arduino.h>

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

int startSeconds = 25; //This will change to the lockdown count
long countdown = startSeconds * 100;

void setup() {
    for(int i = 0; i < 8; i++) pinMode(segPins[i], OUTPUT);
    for(int i = 0; i < 4; i++) pinMode(digitPins[i], OUTPUT);
}

void loop() {

    // update display as fast as possible
    displayNumber(countdown);

    static unsigned long last = 0;
    if (millis() - last >= 10) {
        last = millis();
        if (countdown > 0) countdown--;
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

void setSegments(int digit, bool dp) {
    for (int i = 0; i < 8; i++) {
        int val = numberPattern[digit][i];
        if (i == 7 && dp) val = 1;
        digitalWrite(segPins[i], val);
    }
}