#include <Arduino.h>
#include <WiFiS3.h>
//10.159.167.38 BOARD B IP

//PINS IN USE
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;
const int buttonUnlock = 4;
const int buttonLock = 2;
const int potPin = A0;

// SPECIFICATIONS
const int A = 3 * 1000;     // solid seconds
const int B = 6;            // Hz / flash interval
const int C = 4 * 1000;     // flashing seconds
const int D = 5 * 1000;     // solid seconds
const int E = 3;            // Hz / flash interval
const int f = 4 * 1000;     // flashing seconds
const int G = 4;            // Hz / flash interval
const int H = 2 * 1000;     // flashing seconds

/*  The two parts below are enums which is essentially a fancy type of array that can't
  change in size and the value also can not change. They're good for storing data when you know everything
  and have a fixed size
*/

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

//  WIFI STATES
enum WiFiStates {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED
};

// WIFI STUFF
const char* ssid = "vivo S19 Pro";
const char * password = "88888888";

IPAddress boardB_IP(10, 159, 167, 38);
const int boardB_Port = 8080;

WiFiClient client;

WiFiStates wifiState = WIFI_DISCONNECTED;
bool systemReady = false;
unsigned long lastWiFiAttempt = 0;
const unsigned long wifiRetryInterval = 10000;

// MISS-EL-AIN-EOUS
SystemState currentState = UNLOCKED;
unsigned long lastStateChangeTime = 0;
unsigned long lastBlinkTime = 0;
bool LEDStatus = LOW;

// SERIAL REPORTING 
unsigned long lastReportTime = 0;
const long reportInterval = 1000;
String lastInput = "None";
String activeLED = "Green";

// BUTTON SPAM PREVENTOR
int lastButtonLockState = LOW;
int lastButtonUnlockState = LOW;
bool isLastButtonLock = false;

// PASSWORD STUFF
const String masterPassword = "password";
bool authenticated = false;

/*  Solid LED function
  takes 3 ints for each colour on the rgb led
  they will be HIGH or LOW depending on the colour we want
  take the current potentiometer value and maps it to the pwm 
  wavelength
  then analogWrite because it's not an either/or output
  for each colour pin if red is true(HIGH) it will be the mapped value
  or it will be 0 if false (LOW)
*/ 
void setSolidRGB(int red, int green, int blue) {
  int potValue = analogRead(potPin);
  int brightness = map(potValue, 0, 1023, 0, 255);

  analogWrite(redPin, red == HIGH ? brightness : 0);
  analogWrite(greenPin, green == HIGH ? brightness : 0);
  analogWrite(bluePin, blue == HIGH ? brightness : 0);

  LEDStatus = LOW;
  lastBlinkTime = millis();
}

/*  Blink LED function
  functions the same as the solid but takes an extra variable being our flash rate
  this is time dependant which means we will need to use millis to keep it from blocking our code
  calculate our blink frequency and then check if the current time - the time the function was called is 
  larger than our blink speed if it is, we change the LEDStatus to it's opposite, then the rest is essentially 
  the same as the "Solid LED function"
 */
void setBlinkRGB(int red, int green, int blue, unsigned long frequencyHz) {
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

/*  LED name grabber
  This will get the name of our systems state based off our systemState variable and will return 
  the appropriate String
*/
String getLEDName(SystemState s) {
    switch(s) {
      case UNLOCKED: return "Green Unlocked";
      case GREEN_SOLID: return "Green Solid";
      case GREEN_BLINK: return "Green Blink";
      case YELLOW_SOLID: return "Yellow Solid";
      case YELLOW_BLINK: return "Yellow Blink";
      case RED_LOCKED: return "Red Locked";
      case RED_BLINK: return "Red Blink";
    }
    return "Unknown";
  }

  /*  Sending message to Board B
    To send a message we've set up a Slave Master system between this board (Board A) and
    a second board (Board B). This means that our master board will send commands to the slave board
    and will not receive any commands or responses back. Trying to add in a call and response is difficult 
    and causes the code to lag severly.
    In this we take a command with ours being "LOCK" and "UNLOCK" and then if our board is connected to wifi
    and is able to connect to board B through it's IP and port. It will send the message and then stop.
  */
void sendMessageToBoardB(const char* command) {
  if(wifiState == WIFI_CONNECTED && client.connect(boardB_IP, boardB_Port)) {
      client.println(command);
      client.stop();
      if (authenticated) {
          Serial.print("Sent to board B: ");
          Serial.println(command);
      }
  }
}

/*  Connecting to WiFi fucntion
  This is another time dependant function so we will use millis() to avoid blocking
  our second enum wifiState will help simplify the flow and direction of the code through a 
  switch. 
  - Case 1 is if it's not connected this will always run first and it will begin the process to 
  sign into the wifi connection with the name and password and then set the new state to connecting.

  - Case 2 is our connecting state where it checks if our wifi status is Connected if it is then we continue to 
  case 4 and set systemReady to true which is a starting condition for the main code. It also changes the 
  LED to green and sets our SystemState enum to UNLOCKED. If it isnt connected and the connection time has passed
  since it's last attempt it will set the wifi state to failed

  - Case 3 This one is simple it will check if the time that's passed has been greater than our cooldown interval
  if it has then we reset back to case 1 our disconnected state

  - Case 4 This is the final State which will remain as long as it's still connected to the wifi, if
  at any point it disconnects it will set the state back to case 1 and our systemReady to false, which
  will block the function of our main code

  The second switch state purely deals with the LED during this connection system to make it clear
  to the user what's happening whilst it's first booting up, it will either do nothing or will blink blue
  during the connecting case or stay solid if it's failed it will remain that way until case 2's 
  first if is true where it sets the colour to green for UNLOCK
*/
void connectToWiFi() {
  //HAVE LIGHT BLUE FOR WIFI BECAUSE I FEEL LIKE IT
  unsigned long currentMillis = millis();

  switch(wifiState) {
      case WIFI_DISCONNECTED:
          WiFi.begin(ssid, password);
          wifiState = WIFI_CONNECTING;
          lastWiFiAttempt = currentMillis;
          if (authenticated) {
              Serial.println("Starting WiFi connection...");
          }
          break;
      case WIFI_CONNECTING:
          if (WiFi.status() == WL_CONNECTED) {
              wifiState = WIFI_CONNECTED;
              systemReady = true;
              if (authenticated) {
                  Serial.println("WiFi connected");

              }
              setSolidRGB(LOW, HIGH, LOW);
              currentState = UNLOCKED;
          } else if (currentMillis - lastWiFiAttempt > 15000) {
              wifiState = WIFI_FAILED;
              if (authenticated) {
                  Serial.println("WiFi connection failed");
              }
          }
          break;
      case WIFI_FAILED:
          if (currentMillis - lastWiFiAttempt > wifiRetryInterval) {
              wifiState = WIFI_DISCONNECTED;
          }
          break;
      case WIFI_CONNECTED:
          if (WiFi.status() != WL_CONNECTED) {
              wifiState = WIFI_DISCONNECTED;
              systemReady = false;
              if (authenticated) {
                  Serial.println("WiFi connection lost");
              }
          }
          break;
  }

  switch(wifiState) {
      case WIFI_DISCONNECTED:
      case WIFI_FAILED:
          setSolidRGB(LOW, LOW, HIGH);
          break;
      case WIFI_CONNECTING:
          if (currentMillis % 1000 < 500) {
              setSolidRGB(LOW, LOW, HIGH);
          } else {
              setSolidRGB(LOW, LOW, 100);
          }
          break;
      case WIFI_CONNECTED:
          break;
  }
}

/*  Set up function
  This is simple it will set our LED pins to OUTPUT and our buttons to INPUT,
  initalise the serial monitor and set the LED to blue for the wifi connection phase, 
  also setting the wifiState to disconnected
*/
 void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  pinMode(buttonUnlock, INPUT);
  pinMode(buttonLock, INPUT);

  Serial.begin(9600);
   
  setSolidRGB(LOW, LOW, HIGH);
  wifiState = WIFI_DISCONNECTED;

  Serial.println("System started. Please enter password to see Serial Information. ");
 }

/*  Main loop function
  This is going to be long

  First we will run the connect to WiFi function, then the if statement will block the rest of the code
  unless the systemReady is true or the wifiState is connected

  Then we inialise a bunch of variables used later

  Then we have the password system which will not let the serial monitor print ANYTHING
  unless authenticated is true, it will check if it's not authenticated and there is space in the Serial monitor
  followed by reading any incoming String and will stop reading once a '\n' is entered (the enter key)
  it will trim any spaces and other junk that might get in the way and then it will check if the String entered
  matches our masterPassword.

  explain the rest as u go down
*/

 void loop() {

  connectToWiFi();

  if (!systemReady || wifiState != WIFI_CONNECTED) {
      return;
  }

  int lockButtonState = digitalRead(buttonLock);
  int unlockButtonState = digitalRead(buttonUnlock);

  if (!authenticated && Serial.available() > 0) {
    String incomingStr = Serial.readStringUntil('\n');
    incomingStr.trim();

    if (incomingStr.equals(masterPassword)) {
      authenticated = true;
      Serial.println("Password accepted have fun!");
      lastReportTime = millis();
    } else {
      Serial.println("Password incorrect please try again");
    }
    //  Clears serial buffer
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
  /*  Button Press checking
    It will check if there are 4 things which are true before it will take a button press as valid
    1. The button is reading a HIGH value
    2. The last input the button was reading was LOW
    3. For unlock it checks if the state is NOT UNLOCK and for lock it checks that the state is UNLOCKED
    4. It checks if the last button pressed is true or false (unlocked is true and locked is false)
    This makes sure that the button is reading the correct input we want, it prevents it from reading it 
    multiple times if it's held down, it prevents it from being valid during a state it shouldn't be
    and also makes sure that you can't input the wrong thing (lock button after you've already pressed it)

    - UNLOCKED
    if the conditions are all valid then we initalise some variables used later, set the last input, then
    If the system is locked it will run the blink code else it will just cancel the lock down and set it back
    to UNLOCKED. It will then send a command to board B which will start the timer.

    - LOCKED
    This is more simple than unlocked, it does the same thing except it doesn't have to deal with what the 
    state is because it can only be a valid Lock button press if the System is in UNLOCKED. Everything
    else is the same
  */
  if (unlockButtonState == HIGH && lastButtonUnlockState == LOW && currentState != UNLOCKED && isLastButtonLock) {
    lastStateChangeTime = millis();
    lastBlinkTime = millis();
    lastInput = "Unlock";

    if (currentState == RED_LOCKED) {
      currentState = RED_BLINK;
    } else {
      currentState = UNLOCKED;
    }

    sendMessageToBoardB("UNLOCK");

    if (authenticated) {
      Serial.println(">>> UNLOCK BUTTON PRESSED. STARTING PROTOCOL...");
    }
    isLastButtonLock = false;
  } 

  if (lockButtonState == HIGH && lastButtonLockState == LOW && currentState == UNLOCKED && !isLastButtonLock) {
      
    currentState = GREEN_SOLID;
    lastStateChangeTime = millis();
    lastInput = "Lock";

    sendMessageToBoardB("LOCK");

    if (authenticated) {
      Serial.println(">>> LOCK BUTTON PRESSED. STARTING PROTOCOL...");
    }
    isLastButtonLock = true;
  }
  /*  Our switch statement that will run through the lock down 'protocol' it will basically run through each phase
    For the amount of time needed and will call the SolidRGB or blinkRGB depending on the phase
  */
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
      setBlinkRGB(LOW, HIGH, LOW, B);
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
      setBlinkRGB(HIGH, HIGH, LOW, E);
      if (millis() - lastStateChangeTime >= f) {
        currentState = RED_LOCKED;
        lastStateChangeTime = millis();
      }
      break;
    case RED_LOCKED:
      setSolidRGB(HIGH, LOW, LOW);
        break;
    case RED_BLINK:
      setBlinkRGB(HIGH, LOW, LOW, G);
      if (millis() - lastStateChangeTime >= H) {
        currentState = UNLOCKED;
        lastStateChangeTime = millis();
      }
      break;
  }
  /*  Serial Reporting
    This is in charge of printing our serial information every 1 second,
    It will only run if the password has been authenticated and the time since the last message has been more than a second
    
  */

  if (authenticated) {
    unsigned long currentMillis = millis();

    if (currentMillis - lastReportTime >= reportInterval) {
      int stateNumber = (int)currentState;
      activeLED = getLEDName(currentState);
      lastReportTime = currentMillis;

      Serial.print("State: ");
      Serial.print(stateNumber + 1);//starts from 0 so add 1 
      if (stateNumber != (int)RED_LOCKED && stateNumber != (int)RED_BLINK) {
        Serial.print(" | Unlocked");
      } else {
        Serial.print(" | Locked");
      }

      Serial.print("   | Last Button: ");
      Serial.print(lastInput);
      Serial.print("   | Status Light: ");
      Serial.print(activeLED);
      Serial.print("   | Reading: ");
      Serial.print(analogRead(potPin));
      Serial.print("    | WiFi: ");
      Serial.println(wifiState == WIFI_CONNECTED ? "Connected" : "Disconnected");
    }
  }
  
  lastButtonLockState = lockButtonState;
  lastButtonUnlockState = unlockButtonState;
}