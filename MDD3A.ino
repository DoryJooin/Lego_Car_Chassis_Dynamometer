//LEGO CAR CHASSIS DYNAMOMETER

#include <CytronMotorDriver.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Wire.h>

//핀번호 설정
const int button = 7;            //버튼 : 디지털 7번
const int ir = 17;               //적외선센서 : 아날로그 3번
const int m1a = 3;               //DC모터 M1A핀 : 디지털 3번
const int m1b = 5;               //DC모터 M1B핀 : 디지털 5번
const int m2a = 9;               //DC모터 M2A핀 : 디지털 9번
const int m2b = 11;              //DC모터 M2B핀 : 디지털 11번
const int sda = 18;              //LCD SDA : 아날로그 4번
const int scl = 19;              //LCD SCL : 아날로그 5번
CytronMD m1(PWM_PWM, m1a, m1b);  //DC모터
CytronMD m2(PWM_PWM, m2a, m2b);  //쿨링팬
LiquidCrystal_I2C lcd(0x27, 20, 4);

//각종 변수 설정
int state = 1;                       //현재상태(0:RUN, 1:STBY, 2:OVRSPD TRIP, 3:LOWSPD TRIP)
int curIr = 0;                       //적외선센서 측정횟수가 연속해서 올라가는걸 방지하기 위한 변수
int preIr = 0;                       //적외선센서 측정횟수가 연속해서 올라가는걸 방지하기 위한 변수
unsigned long curTime = 0;           //RPM 측정용 변수
unsigned long preTime = 0;           //RPM 측정용 변수
unsigned long timeBuffer[10];        //시간차 저장할 배열
unsigned long timeSum = 0;           //시간차 배열 총합
unsigned long rpm = 0;               //평균값으로 보정한 RPM
unsigned long lowSpeedLimit = 100;   //저속정지설정치
unsigned long highSpeedLimit = 400;  //고속정지설정치
int lowSpeedTrip = 0;                //저속정지 확인용 변수
int highSpeedTrip = 0;               //고속정지 확인용 변수
unsigned long lowSpeedTripTime;      //저속정지 지연시간 측정용 변수
unsigned long highSpeedTripTime;     //고속정지 지연시간 측정용 변수
unsigned long rev = 0;               //이번 회전수
unsigned long totalRev = 0;          //전체 회전수(EEPROM)
int address = 0;                     //EEPROM에 회전수 저장할 주소
bool cmd = false;                    //모터 기동, 정지 커맨드(true일때만 기동, 정지)
const int speed = -192;              //최대속도(부호는 회전방향)
int curButton;                       //현재 버튼 상태
int preButton = 1;                   //이전 버튼 상태(풀업 초기값 1)
unsigned long runTime = 0;           //일시정지한 시간 제외하고 운전한 시간(트립시 초기화)
unsigned long startTime = 0;         //측정을 시작한 시간(시간저장기능)
unsigned long day;                   //운전시간 표시용 일
unsigned long hour;                  //운전시간 표시용 시간
unsigned long minute;                //운전시간 표시용 분
unsigned long second;                //운전시간 표시용 초
unsigned long lcdCurTime = 0;        //LCD 갱신용 변수
unsigned long lcdPreTime = 0;        //LCD 갱신용 변수
int lcdSign = 0;                     //LCD RUN 기호 표시용 변수

//역슬래시 표시용 문자 정의
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
  //시리얼 초기화
  Serial.begin(9600);
  Wire.setClock(100000);
  //Serial.println("Start");

  //핀 배정
  pinMode(button, INPUT_PULLUP);
  pinMode(ir, INPUT);
  pinMode(m1a, OUTPUT);
  pinMode(m1b, OUTPUT);
  pinMode(m2a, OUTPUT);
  pinMode(m2b, OUTPUT);
  pinMode(sda, OUTPUT);
  pinMode(scl, OUTPUT);

  //기기 초기화
  delay(50);
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, backSlash);
  m1.setSpeed(0);
  m2.setSpeed(0);

  //시간차 저장할 배열 초기화
  for (int i = 0; i < 10; i++) {
    timeBuffer[i] = 0;
  }

  //EEPROM에서 전체 회전수 가져오기
  EEPROM.get(address, totalRev);
}

void loop() {
  //버튼 토글
  curButton = digitalRead(button);
  if (curButton == 0) {
    if (preButton == 1) {
      if (state == 0) {
        //RUN상태면 STBY로 바꾸고 트립 관련 변수 초기화, EEPROM에 전체 회전수 저장
        state = 1;
        lowSpeedTrip = 0;
        highSpeedTrip = 0;
        EEPROM.put(address, totalRev);
        cmd = true;
      } else if (state == 1) {
        //STBY상태면 RUN으로
        state = 0;
        cmd = true;
      } else {
        //TRIP시 STBY로, 시간 및 트립 관련 변수 초기화
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

  // 모터 기동, 정지
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
    //트립에 의한 정지 및 EEPROM에 전체 회전수 기록
    m1.setSpeed(0);
    m2.setSpeed(0);
    EEPROM.put(address, totalRev);
  }

  //회전수, RPM 계산
  if (analogRead(ir) < 500) {
    curIr = 1;
  } else {
    curIr = 0;
  }
  if (curIr == 1 && preIr == 0) {
    preTime = curTime;
    curTime = millis();
    //짧은 시간동안 2번 신호 들어오는것 방지(600rpm 이하에서만 사용가능)
    if (curTime - preTime > 100) {
      rev++;
      totalRev++;
      //RPM 계산
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
  //3초간 적외선센서 신호 없으면 정지한것으로 간주
  if (millis() - curTime > 3000) {
    rpm = 0;
  }

  //운전시간 계산
  if (state == 0) {
    //RUN일때 시간 측정
    runTime = millis() - startTime;
    day = (runTime / 86400000);
    hour = (runTime / 3600000) % 24;
    minute = (runTime / 60000) % 60;
    second = (runTime / 1000) % 60;
  } else if (state > 0) {
    //STBY, TRIP에서는 시간 측정하지 않고 RUN에서 측정한 시간 저장
    startTime = millis() - runTime;
  }

  //RPM에 따른 트립(고속, 저속 설정치 도달 후 3초 후에도 그 상태면 트립)
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

  //LCD 화면표시
  lcdCurTime = millis();
  if (lcdCurTime - lcdPreTime > 500) {
    lcdPreTime = lcdCurTime;
    lcdSign = (lcdSign + 1) % 4;
    //첫번째줄 : 상태
    lcd.setCursor(0, 0);
    lcd.print("STATE: ");
    //Serial.print("STATE: ");  //***테스트***
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
      //Serial.println("RUN");  //***테스트***
    } else if (state == 1) {
      lcd.print("STBY       ");
      //Serial.println("STBY");  //***테스트***
    } else if (state == 2) {
      lcd.print("OVRSPD TRIP");
      //Serial.println("OVRSPD TRIP");  //***테스트***
    } else if (state == 3) {
      lcd.print("LOWSPD TRIP");
      //Serial.println("LOWSPD TRIP");  //***테스트***
    }

    //두번째줄 : RPM
    lcd.setCursor(0, 1);
    lcd.print("RPM: ");
    lcd.print(rpm);
    for (int i = 0; i < 15 - String(rpm).length(); i++) {
      lcd.print(" ");
    }
    /*
    Serial.print("RPM: ");  //***테스트***
    Serial.println(rpm);    //***테스트***
    */

    //세번째줄 : 운전시간
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
    Serial.println(runTime);    //***테스트***
    Serial.print("RUNTIME: ");  //***테스트***
    Serial.print(day);          //***테스트***
    Serial.print(":");          //***테스트***
    Serial.print(hour);         //***테스트***
    Serial.print(":");          //***테스트***
    Serial.print(minute);       //***테스트***
    Serial.print(":");          //***테스트***
    Serial.println(second);     //***테스트***
    */

    //네번째줄 : 이번 회전수, 전체 회전수
    lcd.setCursor(0, 3);
    lcd.print("REV: ");
    lcd.print(rev);
    lcd.print(" | ");
    lcd.print(totalRev);
    for (int i = 0; i < 12 - String(rev).length() - String(totalRev).length(); i++) {
      lcd.print(" ");
    }
    /*
    Serial.print("REV: ");     //***테스트***
    Serial.print(rev);         //***테스트***
    Serial.print(" | ");       //***테스트***
    Serial.println(totalRev);  //***테스트***
    */
  }
}