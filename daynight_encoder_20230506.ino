// This is an LED controller for two LED Strips (one RGB, one WW/CW) controlled by an encoder.
// The controller simulates a 24-hour day-night cycle; each LED channel is hard coded with a
// specific value for each hour.  
//
// The controller can operate in two modes: MANUAL and CYCLE.
//
// In MANUAL mode, the encoder selects the hour for the LEDs to display. The lights are locked
// to that hour until the knob is turned or the button on the encoder is pressed, switching it
// to CYCLE mode.
//
// In CYCLE mode, the program runs a simulated clock, with the encoder controlling the speed.
// Speed is variable in detents from real time (1min/min) to "demo mode"
// Every simulated minute, the value of every LED channel is updated to smoothly transition
// from one minute to the next. The LEDs will cycle until the button is pressed, returning it
// to MANUAL mode.
//
// All of the RGB/WC values, as well as the current sim time, are printed for diagnostics
// and export to other devices. 

#include <EncoderButton.h>
#include <LiquidCrystal.h>

#define cPin 6
#define wPin 5
#define rPin 10
#define gPin 9
#define bPin 11

EncoderButton eb1(2, 3, 4); //DT/CLK?

const int rs = A5, en = A4, d4 = A3, d5 = A2, d6 = A1, d7 = A0; //LCD screen pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

bool modeManual = true;
bool encoderTurned = false;
int encoderIncrement;
short hour = 5;
short minute = 0;
unsigned long previousMillis = 0;
int fadeMultiplier = 1;

void onEb1Pressed(EncoderButton& eb) {
  modeManual = !modeManual;
  Serial.println("Mode changed.");
}

void onEb1Encoder(EncoderButton& eb) {
  encoderTurned = true;
  encoderIncrement = eb.increment();
}

void setup() {

  //Start serial comms (optional)
  Serial.begin(9600);
  Serial.println("Controller ready!");

  //Encoder button settings
  eb1.setPressedHandler(onEb1Pressed);
  eb1.setEncoderHandler(onEb1Encoder);

  //Set up the LED pins
  pinMode(cPin, OUTPUT);
  pinMode(wPin, OUTPUT);
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);

  // Set up the LCD pins and resolution
  pinMode(rs, OUTPUT);
  pinMode(en, OUTPUT);
  pinMode(d4, OUTPUT);
  pinMode(d5, OUTPUT);
  pinMode(d6, OUTPUT);
  pinMode(d7, OUTPUT);
  lcd.begin(16, 2);
}

// LED Update function
void setLEDStates() {
  for (int LED = 0; LED < 5; LED++) {
    short nextHour = hour + 1;
    if (nextHour == 24) {
      nextHour = 0;
    }
    const int pinTable[] = {wPin, cPin, rPin, gPin, bPin}; //LEDs will be written to in this order 
    const byte lightTable[][24] = { //this array stores all light states. Row = LED Channel. Column = Hour.
    //0  1 2 3 4 5 6  7   8   9   10  11  12  13  14  15  16  17  18  19 20 21 22 23
     {0 ,0,0,0,0,0,0 ,8  ,128,255,255,255,255,255,255,255,255,128,8  ,0 ,0 ,0 ,0 ,0}, //warm
     {0 ,0,0,0,0,0,0 ,0  ,64 ,128,255,255,255,255,255,255,128,0  ,0  ,0 ,0 ,0 ,0 ,0}, //cool
     {5 ,6,5,4,3,2,16,128,64 ,64 ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,128,128,16,4 ,5 ,6 ,6}, //red
     {16,4,2,1,0,0,0 ,0  ,32 ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,64 ,0  ,0 ,0 ,1 ,2 ,4}, //green
     {6 ,9,7,5,3,1,16,0  ,64 ,128,255,255,255,255,255,255,255,32 ,0  ,8 ,8 ,8 ,9 ,10} //blue
    };
    byte ledValue;
    int target = lightTable[LED][nextHour];
    int start = lightTable[LED][hour];
    float fMinute = minute;

    ledValue = (target-start)*(fMinute/60)+start;
    analogWrite(pinTable[LED],ledValue);
    //Serial.print(ledValue);
    //Serial.print("/");
  }
}

//Serial monitor print function
void printTime() {
  Serial.println("Time is now ");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) {
    Serial.print("0");
    Serial.println(minute);
  }
  else {
    Serial.println(minute);
  }
}

//LCD Display function
void printToLCD(){
  lcd.setCursor(2,0);
  if (hour < 10) {
    lcd.print(" ");
  }
  lcd.print(hour);
  lcd.print(":");
  if (minute < 10) {
    lcd.print("0");
    lcd.print(minute);
  }
  else {
    lcd.print(minute);
  }
  if (modeManual == true) {
    lcd.setCursor(0,1);
    lcd.print("  MODE: MANUAL  ");
  } 
  else {
    lcd.setCursor(9,0);
    lcd.print(fadeMultiplier);
    lcd.print("X");
    lcd.setCursor(0,1);
    lcd.print("  MODE: CYCLE   ");
  }
}

void loop() {;
  printToLCD();
  eb1.update();

  if (modeManual == true) {
    if (encoderTurned == true) {
      encoderTurned = false;
      minute = 0;      
      hour = hour + encoderIncrement;
      if (hour > 23) {
        hour = 0;
      }
      if (hour < 0) {
        hour = 23;
      }
      //setLEDStates();
      //printTime();
      //printToLCD();
    }
  }

  if (modeManual == false) {
    unsigned int fadeSpeed;
    unsigned long currentMillis = millis();

    if (encoderTurned == true) {
      encoderTurned = false;
      fadeMultiplier = fadeMultiplier + encoderIncrement;
      if (fadeMultiplier >= 11) {
        fadeMultiplier = fadeMultiplier + 9*encoderIncrement;
      }
      fadeMultiplier = constrain(fadeMultiplier,1,500);
      fadeSpeed = 60000/fadeMultiplier;
      fadeSpeed = constrain(fadeSpeed,100,60000);
    }

    if (currentMillis - previousMillis >= fadeSpeed) {
      previousMillis = currentMillis;
      minute++;
      if (minute > 59) {
        minute = 0;
        hour++;
        if (hour > 23) {
          hour = 0;
        }
      }
    }

  }
  setLEDStates();
  //printTime();
}
