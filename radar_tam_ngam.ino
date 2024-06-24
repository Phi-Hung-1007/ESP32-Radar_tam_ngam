#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Servo.h>

#define SR04_TRIG_PIN 18    // Chân GPIO cho Trig của SR04
#define SR04_ECHO_PIN 19    // Chân GPIO cho Echo của SR04
#define I2C_ADDRESS 0x3c    // Địa chỉ I2C cho OLED
#define SCREEN_WIDTH 128    // Chiều rộng màn hình OLED, tính bằng pixel
#define SCREEN_HEIGHT 64    // Chiều cao màn hình OLED, tính bằng pixel
#define OLED_RESET -1       // Chân reset OLED

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Servo myservo;              // Tạo đối tượng servo để điều khiển servo
int servoPin = 23;          // Chân PWM GPIO cho servo
int pos = 0;                // Biến để lưu vị trí của servo
unsigned int distance;      // Khoảng cách từ cảm biến siêu âm
String angle = "";
String distanceStr = "";
int iAngle = 0;
int iDistance = 0;
int cmd;
int mode = 0;
int radius = 40; 

void setup() {
  pinMode(SR04_TRIG_PIN, OUTPUT);
  pinMode(SR04_ECHO_PIN, INPUT);

  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 - Radar tầm ngắm");

  display.begin(I2C_ADDRESS, true); // Địa chỉ 0x3C mặc định
  display.setContrast(0); // Giảm độ sáng màn hình nếu cần

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myservo.setPeriodHertz(50);       // Servo chuẩn 50 Hz
  myservo.attach(servoPin, 550, 2500); // Gắn servo vào chân 18
}

int layKhoangCach() {
  digitalWrite(SR04_TRIG_PIN, LOW);   // Đặt Trig xuống mức thấp trong 2µS
  delayMicroseconds(2);
  digitalWrite(SR04_TRIG_PIN, HIGH);  // Gửi xung siêu âm trong 10µS
  delayMicroseconds(10);
  digitalWrite(SR04_TRIG_PIN, LOW);   // Dừng gửi xung
  unsigned int microseconds = pulseIn(SR04_ECHO_PIN, HIGH); // Đợi phản hồi
  return microseconds / 58;           // Tính khoảng cách bằng cm
}

void veVongCung(int x, int y, int radius, int startAngle, int endAngle, int color) {
  for (int angle = startAngle; angle < endAngle; angle++) {
    int x0 = x + radius * cos(radians(angle));
    int y0 = y + radius * sin(radians(angle));
    display.drawPixel(x0, y0, color);
  }
}

void veRadar() {
  int radarCenterX = SCREEN_WIDTH / 2; // Tọa độ trung tâm của radar theo chiều ngang
  int radarCenterY = SCREEN_HEIGHT; // Tọa độ trung tâm của radar theo chiều dọc (đáy màn hình)

  display.clearDisplay();
  for (int i = 5; i < 60; i += 10){
    veVongCung(radarCenterX, radarCenterY, i, 180, 360, SH110X_WHITE);
  }
  // Vẽ các đường tỏa
  for (int angle = 30; angle <= 150; angle += 30) {
    int x = radarCenterX + cos(radians(angle)) * 55;
    int y = radarCenterY - sin(radians(angle)) * 55;
    display.drawLine(radarCenterX, radarCenterY, x, y, SH110X_WHITE);
  }
  display.display();
}

void veDoiTuong() {
  float khoangCachPixel = iDistance * (55.0 / radius); // Chuyển đổi khoảng cách thành pixel
  if (iDistance < radius) {
    int x = SCREEN_WIDTH / 2 + khoangCachPixel * cos(radians(iAngle));
    int y = SCREEN_HEIGHT - khoangCachPixel * sin(radians(iAngle));
    display.fillCircle(x, y, 2, SH110X_WHITE);
  }
  display.display();
}

void veChu() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  
  if (iDistance > radius) {
    display.print("Scanning...");
  } else {
    display.print("Ditection: ");
    display.print(iDistance);
    display.print("cm-");
    display.print(iAngle);
    display.print("do");
  }
  display.display();
}

void menu() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.print("1. Start");
  display.setCursor(0, 30);
  display.print("2. Controller");
  display.display();
}

void controller() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.print("Set radius: ");
  display.print(radius);
  display.setCursor(0, 15);
  display.print("Increase: +");
  display.setCursor(0, 30);
  display.print("Decrease: -");
  display.setCursor(0, 45);
  display.print("Exit: 0");
  display.display();
}

void quetRadar(){
  myservo.write(iAngle);
  delay(15);
  iDistance = layKhoangCach();
  veRadar();
  veDoiTuong();
  veChu();
  yield();
}

void loop() {
  if (Serial.available() > 0) {
    cmd = Serial.read();
    if (cmd == '1') {
      mode = 1; // Chuyển sang chế độ đo khoảng cách
    } else if (cmd == '2') {
      mode = 2; // Chuyển sang chế độ controller
    } else if (cmd == '0') {
      mode = 0; // Chuyển sang chế độ menu
      menu(); // Vẽ menu ngay lập tức
    } else if (mode == 2 && (cmd == '+' || cmd == '-')) {
      if (cmd == '+') {
        radius += 10; // Tăng bán kính
      } else if (cmd == '-') {
        radius -= 10; // Giảm bán kính
        if (radius < 10) radius = 10; // Đảm bảo bán kính không âm
      }
      controller(); // Cập nhật màn hình controller
    }
  }

  if (mode == 1) {
    for (pos = 0; pos <= 180; pos += 1) {
      if (Serial.available() > 0) {
        cmd = Serial.read();
        if (cmd == '0') {
          mode = 0; // Chuyển sang chế độ menu
          menu();
          return; // Thoát vòng lặp và trở về chế độ menu
        }
      }
      iAngle = pos;
      quetRadar();
    }
    for (pos = 180; pos >= 0; pos -= 1) {
      if (Serial.available() > 0) {
        cmd = Serial.read();
        if (cmd == '0') {
          mode = 0; // Chuyển sang chế độ menu
          menu();
          return; // Thoát vòng lặp và trở về chế độ menu
        }
      }
      iAngle = pos;
      quetRadar();
    }
  } else if (mode == 0){
    menu();
  } else if (mode == 2) {
    controller();
  }
}
