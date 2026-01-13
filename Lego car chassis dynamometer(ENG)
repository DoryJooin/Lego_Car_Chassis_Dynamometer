//LEGO CAR CHASSIS DYNAMOMETER

#include <CytronMotorDriver.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Wire.h>

//pin mapping
const int button = 7;            //button : digital 7
const int ir = 17;               //infrared sensor : analog 3
const int m1a = 3;               //DC motor M1A pin : digital 3
const int m1b = 5;               //DC motor M1B pin : digital 5
const int m2a = 9;               //DC motor M2A pin : digital 9
const int m2b = 11;              //DC motor M2B pin : digital 11
const int sda = 18;              //LCD SDA : analog 4
const int scl = 19;              //LCD SCL : analog 5
CytronMD m1(PWM_PWM, m1a, m1b);  //DC motor
CytronMD m2(PWM_PWM, m2a, m2b);  //cooling fan
LiquidCrystal_I2C lcd(0x27, 20, 4);

//variables setting
int state = 1;                       //current state(0:RUN, 1:STBY, 2:OVRSPD TRIP, 3:LOWSPD TRIP)
int curIr = 0;                       //variable to prevent IR sensor count raising continuously
int preIr = 0;                       //variable to prevent IR sensor count raising continuously
unsigned long curTime = 0;           //variable for RPM count
unsigned long preTime = 0;           //variable for RPM count
unsigned long timeBuffer[10];        //array for memorizing time gap
unsigned long timeSum = 0;           //sum of time gap array
unsigned long rpm = 0;               //average corrected RPM
unsigned long lowSpeedLimit = 100;   //low speed trip setpoint
unsigned long highSpeedLimit = 400;  //high speed trip setpoint
int lowSpeedTrip = 0;                //low speed trip check variable
int highSpeedTrip = 0;               //high speed trip check variable
unsigned long lowSpeedTripTime;      //variable for checking low speed trip delay time
unsigned long highSpeedTripTime;     //variable for checking high speed trip delay time
unsigned long rev = 0;               //current revolution(resets to 0 when power's off)
unsigned long totalRev = 0;          //total revolution(saved to EEPROM)
int address = 0;                     //EEPROM total revolution save address
bool cmd = false;                    //motor start/stop command(only starts/stops when this variable is true)
const int speed = -192;              //maximum speed(plus/minus sign is for rotation direction)
int curButton;                       //current button state
int preButton = 1;                   //previous button state(initial value for pullup is 1)
unsigned long runTime = 0;           //runtime without standby time(resets when trip)
unsigned long startTime = 0;         //starting time of runtime(for memorizing operation time)
unsigned long day;                   //day of runtime
unsigned long hour;                  //hour of runtime
unsigned long minute;                //minute of runtime
unsigned long second;                //second of runtime
unsigned long lcdCurTime = 0;        //variable for LCD update
unsigned long lcdPreTime = 0;        //variable for LCD update
int lcdSign = 0;                     //variable for LCD RUN sign update

//defining backslash character
byte backSlash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};

void setup() {
  //serial initializing
  Serial.begin(9600);
  Wire.setClock(100000);
  //Serial.println("Start");

  //pin mapping
  pinMode(button, INPUT_PULLUP);
  pinMode(ir, INPUT);
  pinMode(m1a, OUTPUT);
  pinMode(m1b, OUTPUT);
  pinMode(m2a, OUTPUT);
  pinMode(m2b, OUTPUT);
  pinMode(sda, OUTPUT);
  pinMode(scl, OUTPUT);

  //initializing components
  delay(50);
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, backSlash);
  m1.setSpeed(0);
  m2.setSpeed(0);

  //initializing time gap array
  for (int i = 0; i < 10; i++) {
    timeBuffer[i] = 0;
  }

  //bring total revolution from EEPROM
  EEPROM.get(address, totalRev);
}

void loop() {
  //toggling button
  curButton = digitalRead(button);
  if (curButton == 0) {
    if (preButton == 1) {
      if (state == 0) {
        //if the state is RUN, switch to STBY and reset trip variables, save total revolution to EEPROM
        state = 1;
        lowSpeedTrip = 0;
        highSpeedTrip = 0;
        EEPROM.put(address, totalRev);
        cmd = true;
      } else if (state == 1) {
        //if the state is STBY, switch to RUN
        state = 0;
        cmd = true;
      } else {
        //if the state is TRIP, switch to STBY and reset time, trip variables
        state = 1;
        runTime = 0;
        startTime = 0;
        day = 0;
        hour = 0;
        minute = 0;
        second = 0;
        lowSpeedTrip = 0;
        highSpeedTrip = 0;
      }
    }
  }
  preButton = curButton;

  // motor start, stop
  if (state == 0 && cmd == true) {
    //기동
    m1.setSpeed(speed);
    m2.setSpeed(-255);
    cmd = false;
  } else if (state == 1 && cmd == true) {
    //정지
    m1.setSpeed(0);
    m2.setSpeed(0);
    cmd = false;
  } else if (state > 1) {
    //motor stop by trip. save total revolution to EEPROM
    m1.setSpeed(0);
    m2.setSpeed(0);
    EEPROM.put(address, totalRev);
  }

  //calculating revolution and rpm
  if (analogRead(ir) < 500) {
    curIr = 1;
  } else {
    curIr = 0;
  }
  if (curIr == 1 && preIr == 0) {
    preTime = curTime;
    curTime = millis();
    //infrared sensor noise filtering(maximum measurable rpm is 600)
    if (curTime - preTime > 100) {
      rev++;
      totalRev++;
      //rpm calculating
      for (int i = 9; i > 0; i--) {
        timeBuffer[i] = timeBuffer[i - 1];
      }
      timeBuffer[0] = curTime - preTime;
      timeSum = 0;
      for (int i = 0; i < 10; i++) {
        timeSum += timeBuffer[i];
      }
      rpm = 600000 / timeSum;
    }
  }
  preIr = curIr;
  //if there's no signal from infrared sensor more than 3 seconds, the motor is considered to have stopped
  if (millis() - curTime > 3000) {
    rpm = 0;
  }

  //calculating runtime
  if (state == 0) {
    //measure time if state is RUN
    runTime = millis() - startTime;
    day = (runTime / 86400000);
    hour = (runTime / 3600000) % 24;
    minute = (runTime / 60000) % 60;
    second = (runTime / 1000) % 60;
  } else if (state > 0) {
    //does not measure time when it's STBY, TRIP and just remember time which is measured in RUM mode
    startTime = millis() - runTime;
  }

  //trip by rpm(reaching beyond low/high trip setpoint more than 3 seconds)
  if (state == 0 && rpm > highSpeedLimit) {
    if (highSpeedTrip == 0) {
      highSpeedTrip = 1;
      highSpeedTripTime = millis();
    }
  }
  if (millis() - highSpeedTripTime > 3000 && state == 0 && highSpeedTrip == 1) {
    if (rpm > highSpeedLimit) {
      state = 2;
    } else {
      highSpeedTrip = 0;
    }
  }
  if (state == 0 && rpm < lowSpeedLimit) {
    if (lowSpeedTrip == 0) {
      lowSpeedTrip = 1;
      lowSpeedTripTime = millis();
    }
  }
  if (millis() - lowSpeedTripTime > 3000 && state == 0 && lowSpeedTrip == 1) {
    if (rpm < lowSpeedLimit) {
      state = 3;
    } else {
      lowSpeedTrip = 0;
    }
  }

  //LCD display
  lcdCurTime = millis();
  if (lcdCurTime - lcdPreTime > 500) {
    lcdPreTime = lcdCurTime;
    lcdSign = (lcdSign + 1) % 4;
    //1st line : state
    lcd.setCursor(0, 0);
    lcd.print("STATE: ");
    //Serial.print("STATE: ");  //***test***
    if (state == 0) {
      lcd.print("RUN");
      if (lcdSign == 0) {
        lcd.print("-");
      } else if (lcdSign == 1) {
        lcd.write(1);
      } else if (lcdSign == 2) {
        lcd.print("|");
      } else if (lcdSign == 3) {
        lcd.print("/");
      }
      lcd.print("       ");
      //Serial.println("RUN");  //***test***
    } else if (state == 1) {
      lcd.print("STBY       ");
      //Serial.println("STBY");  //***test***
    } else if (state == 2) {
      lcd.print("OVRSPD TRIP");
      //Serial.println("OVRSPD TRIP");  //***test***
    } else if (state == 3) {
      lcd.print("LOWSPD TRIP");
      //Serial.println("LOWSPD TRIP");  //***test***
    }

    //2nd line : rpm
    lcd.setCursor(0, 1);
    lcd.print("RPM: ");
    lcd.print(rpm);
    for (int i = 0; i < 15 - String(rpm).length(); i++) {
      lcd.print(" ");
    }
    /*
    Serial.print("RPM: ");  //***test***
    Serial.println(rpm);    //***test***
    */

    //3rd line : runtime
    lcd.setCursor(0, 2);
    lcd.print("RUNTIME: ");
    lcd.print(day);
    lcd.print(":");
    lcd.print(hour);
    lcd.print(":");
    lcd.print(minute);
    lcd.print(":");
    lcd.print(second);
    for (int i = 0; i < 8 - String(day).length() - String(day).length() - String(hour).length() - String(minute).length() - String(second).length(); i++) {
      lcd.print(" ");
    }
    /*
    Serial.println(runTime);    //***test***
    Serial.print("RUNTIME: ");  //***test***
    Serial.print(day);          //***test***
    Serial.print(":");          //***test***
    Serial.print(hour);         //***test***
    Serial.print(":");          //***test***
    Serial.print(minute);       //***test***
    Serial.print(":");          //***test***
    Serial.println(second);     //***test***
    */

    //4th line : current revolution, total revolution
    lcd.setCursor(0, 3);
    lcd.print("REV: ");
    lcd.print(rev);
    lcd.print(" | ");
    lcd.print(totalRev);
    for (int i = 0; i < 12 - String(rev).length() - String(totalRev).length(); i++) {
      lcd.print(" ");
    }
    /*
    Serial.print("REV: ");     //***test***
    Serial.print(rev);         //***test***
    Serial.print(" | ");       //***test***
    Serial.println(totalRev);  //***test***
    */
  }
}
