#include <Servo.h> 
#include <IRremote.h>
#include <LiquidCrystal_I2C.h>

#define INTERSECTION_LEFT_PIN 11  // 교차로 왼쪽 감지 센서
#define LEFT_SENSOR_PIN 10 // 왼쪽 적외선 센서
#define RIGHT_SENSOR_PIN 9 // 오른쪽 적외선 
#define INTERSECTION_RIGHT_PIN 8 // 교차로 오른쪽 감지 센서

#define RECV_PIN A0 //리모콘 리시버

Servo servoLeft; 
Servo servoRight;
Servo servoDrop;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// 센서 초깃값: 라인트레이싱 센서는 1, 교차로 감지 센서는 0
int leftSensor = 1;
int rightSensor = 1;
int intersectionLeft = 0;
int intersectionRight = 0;

//리모콘 핀 모드 설정
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  // 서보 모터 초기화
  servoLeft.attach(13); // 왼쪽 서보 신호 핀 13에 연결
  servoRight.attach(12); // 오른쪽 서보 신호 핀 12에 연결
  
  servoDrop.attach(6);
  servoDrop.write(0); 
  delay(500);

  // 센서 핀 모드 설정
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);
  pinMode(INTERSECTION_LEFT_PIN, INPUT);
  pinMode(INTERSECTION_RIGHT_PIN, INPUT);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("DilyDily");
  delay(1000);

  irrecv.enableIRIn();
}

void loop() {
  maneuver(0, 0); 
  int x = -1, y = 0;
  int recv_x, recv_y;
  String lcdStr = "";
  if(irrecv.decode(&results)){
    Serial.println(results.value, HEX);
    switch (results.value){
      case 0xFF30CF: recv_x = 0, recv_y = 0,  lcdStr = "Go 1"; break;    // botton 1
      case 0xFF18E7: recv_x = 1, recv_y = 0,  lcdStr = "Go 2"; break;    // botton 2
      case 0xFF7A85: recv_x = 0, recv_y = 1,  lcdStr = "Go 3"; break;    // botton 3
      case 0xFF10EF: recv_x = 1, recv_y = 1,  lcdStr = "Go 4"; break;    // botton 4
      //case 0xFF38C7: recv_x = 1, recv_y = 1,  lcdStr = "Go 5"; break;    // botton 5
      //case 0xFF5AA5F: recv_x = 2, recv_y = 1,   lcdStr = "Go 6"; break;    // botton 6
      //case 0xFF42BD: recv_x = 0, recv_y = 2,  lcdStr = "Go 7"; break;    // botton 7
      //case 0xFF4AB5: recv_x = 1, recv_y = 2,  lcdStr = "Go 8"; break;    // botton 8
      //case 0xFF52AD: recv_x = 2, recv_y = 2,  lcdStr = "Go 9"; break;    // botton 9
      default: recv_x = -1, recv_y = -1,  lcdStr = "Try again";
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdStr);

    if(recv_x > -1 && recv_y > -1){
      // 센서 값 읽기
      leftSensor = digitalRead(LEFT_SENSOR_PIN);
      rightSensor = digitalRead(RIGHT_SENSOR_PIN);
      intersectionLeft = digitalRead(INTERSECTION_LEFT_PIN);
      intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
      
      maneuver(60, 60); // 처음 실행 시 센서값 오류를 방지하기 위해 일정 거리 전진
      delay(500);

      intersectionLeft = digitalRead(INTERSECTION_LEFT_PIN);
      while(x < recv_x){
        intersectionLeft = digitalRead(INTERSECTION_LEFT_PIN);
        if(intersectionLeft){
          x++;
          if(x<recv_x){
            while(intersectionLeft){
              drive();
              intersectionLeft = digitalRead(INTERSECTION_LEFT_PIN);
            }
          }
        }else{
          drive();
        }
      }

      leftTurn();

      while(y < recv_y){
        intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
        if(intersectionRight){
          y++;
          if(y < recv_y){
            while(intersectionRight){
              drive();
              intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
            }
          }
        }else{
          drive();
        }
      }

      drop(recv_y);

      goHome(recv_y);

      turn180();

    }

    irrecv.resume();
  }
}

void maneuver(int speedLeft, int speedRight){ 
  servoLeft.writeMicroseconds(1500 + speedLeft);   // Set Left servo speed
  servoRight.writeMicroseconds(1500 - speedRight); // Set right servo speed
  delay(10);
}

void drive(){
  leftSensor = digitalRead(LEFT_SENSOR_PIN);
  rightSensor = digitalRead(RIGHT_SENSOR_PIN);

  if (leftSensor == HIGH && rightSensor == HIGH) {
    // 두 센서가 모두 검은색을 감지하면 직진
    maneuver(50, 50);
  } else if (leftSensor == HIGH && rightSensor == LOW) {
    // 왼쪽 센서만 검은색을 감지하면 왼쪽으로 회전
    maneuver(20, 60);
  } else if (leftSensor == LOW && rightSensor == HIGH) {
    // 오른쪽 센서만 검은색을 감지하면 오른쪽으로 회전
    maneuver(60, 20);
  }else if(leftSensor == LOW && rightSensor == LOW){
    maneuver(0, 0);
  }
}

void leftTurn(){
  maneuver(0, 60); // 왼쪽 회전
  delay(1600);
  while (leftSensor == HIGH && rightSensor == HIGH) { // 센서가 모두 검은 선을 인식할 때 까지
    maneuver(0, 60); // 왼쪽 회전
    leftSensor = digitalRead(LEFT_SENSOR_PIN);  // 센서 값 업데이트
    rightSensor = digitalRead(RIGHT_SENSOR_PIN);
  }
}

void rightTurn(){
  maneuver(60, 0); 
  delay(1600);
  while (leftSensor == HIGH && rightSensor == HIGH) { // 센서가 모두 검은 선을 인식할 때 까지
    maneuver(60, 0); // 왼쪽 회전
    leftSensor = digitalRead(LEFT_SENSOR_PIN);  // 센서 값 업데이트
    rightSensor = digitalRead(RIGHT_SENSOR_PIN);
  }
}

void goHome(int recv_y){
  int hy = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Go Home");

  for(int i=0; i<2; i++){
    intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
    while(!intersectionRight){
      drive();
      intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
    }
    rightTurn();
  }
  maneuver(0,0);
  while(hy < recv_y){
    intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
    if(intersectionRight){
      hy++;
      if(hy < recv_y){
        while(intersectionRight){
          drive();
          intersectionRight = digitalRead(INTERSECTION_RIGHT_PIN);
        }
      }
    }else{
      drive();
    }
  }
  rightTurn();
  while(leftSensor || rightSensor){
    drive();
  }
}

void drop(int recv_y){
  if(recv_y > 0){
    maneuver(50,50);
    delay(1500);
  }
  maneuver(50,50);
  delay(1000);

  maneuver(0,0);
  servoDrop.write(105); 
  delay(1000);
  servoDrop.write(0); 
  delay(500);
}

void turn180(){ 
  servoLeft.writeMicroseconds(1700); 
  servoRight.writeMicroseconds(1700); 
  delay(1100);
}