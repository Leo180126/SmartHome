#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>
#include <secrets.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


// Blynk define for include
#define BLYNK_TEMPLATE_ID "TMPL6arNBTNbS"
#define BLYNK_TEMPLATE_NAME "SmartHouseIOT"
#define BLYNK_AUTH_TOKEN "NGXXK9hsZycdreAk_8WIc1q_kPrblQTd"


#include <WiFiClient.h>
#include <BlynkSimpleWifi.h>


// For DHT and Blynk
const int dht11_pin = 32; // Theo Schematic Quyến vẽ thì là pin 6, t chỉ đg test ở đây th

BlynkTimer timer;
DHT_Unified dht(dht11_pin, DHT11);

unsigned long int startup_timer = 0;
float temperature = 0;
float relative_humidity = 0;


// Marco for keypad
const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {13, 16, 14, 27}; // Connect to the row pinouts
byte colPins[COLS] = {26, 25, 33};     // Connect to the column pinouts

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Marco for 74HC595
#define DATA_PIN  15  // DS
#define CLOCK_PIN 04  // SH_CP
#define LATCH_PIN 02   // ST_CP
byte leds = 0b00000000;

// Marco for HC-SR04
#define TRIG_PIN 05    // Trigger pin
#define ECHO_PIN 18   // Echo pin

// Macro for MQ2
#define MQ2_PIN 35

// Macro for SERVO
#define SERVO_PIN 17
Servo myServo;

// For LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Marco for BUZZER
#define BUZZER_PIN 19

// Marco for Fan
#define FAN_PIN 23
bool fanState = false; // True - quay, False - khong quay

// Các biến chính
String password = "123";
String inputPassword = "";
bool doorOpen = false;



// Function decoration
void sendByte(byte val);
void checkKeypad();
void checkGas();
void checkPerson();
void openDoor();
void quayQuat(bool isQuay);
void getDataDHT(float &temp, float &rh);
void wifiConnect();
void send_data();
byte toByte(String a);
void ledDoor(bool state);
byte updateByteFromString(byte original, String str);

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  // Allow allocation of all timers
  myServo.attach(SERVO_PIN, 500, 2400); // SG90: min=500us, max=2400us // Gắn vào GPIO 17, xung min–max
  delay(1000);
  Serial.println("Setup OK");
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Nhap mat khau:");
  // startup_timer = millis();
  wifiConnect();
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  dht.begin();

  timer.setInterval(5000L, send_data);
}

BLYNK_WRITE(V1) {   
  // Called when the datastream virtual pin V1 is updated 
  // by Blynk.Console, Blynk.App, or HTTP API. 

  String LedControl = param.asStr();
  // OR:
  //String value = param.asString();
  byte ledControlByte = toByte(LedControl);
  for(int i=0; i<8; i++){
    
  }
  Serial.print("V1 = '");
  Serial.println(ledControlByte, BIN);
  Serial.println("'");    
  
} // BLYNK_WRITE()

void loop() {
  getDataDHT(temperature, relative_humidity);
  Blynk.run();
  timer.run();
  checkKeypad();
  checkGas();
  checkPerson();
}

// Function definition
void sendByte(byte val) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(DATA_PIN, (val >> i) & 0x01);
    delay(10);  // Delay nhỏ để chắc chắn
    digitalWrite(CLOCK_PIN, HIGH);
    delay(10);
  }

  digitalWrite(LATCH_PIN, LOW);
  delay(10);
  digitalWrite(LATCH_PIN, HIGH);
}
void checkKeypad() {
  char key = keypad.getKey();
  // Serial.println(key);
  if (key) {
    lcd.setCursor(0,1);
    if (key == '#') {
      if (inputPassword == password) {
        lcd.clear();
        lcd.print("Dung mat khau");
        openDoor();
      } else {
        lcd.clear();
        lcd.print("Sai! Thu lai");
        delay(1500);
      }
      inputPassword = "";
      lcd.clear(); lcd.print("Nhap mat khau:");
    } else if (key == '*') {
      inputPassword = "";
      lcd.clear(); lcd.print("Nhap mat khau:");
    } else {
      inputPassword += key;
      lcd.print(inputPassword);
    }
  }
}
void openDoor() {
  myServo.write(90); // mở
  doorOpen = true;
  delay(5000);
  myServo.write(0); // đóng
  doorOpen = false;
  delay(5000);
}
void checkGas() {
  bool isQuay = false;
  int gasValue = analogRead(MQ2_PIN);
  if (gasValue > 2000) { // ngưỡng phát hiện tùy chỉnh
    // digitalWrite(BUZZER_PIN, HIGH);
    // Serial.print("Chay roi: ");
    isQuay = true;
    // Serial.println(gasValue);
  } else {
    // digitalWrite(BUZZER_PIN, LOW);
    // Serial.print("On roi");
    isQuay = false;
    // Serial.println(gasValue);
  }
  if(isQuay != fanState){
    quayQuat(isQuay);
    fanState = isQuay;
    
  }
}
void checkPerson() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  // Serial.println(distance);
  if (distance < 20) {
    ledDoor(1);
    sendByte(leds);// có người
  } else {
    ledDoor(0);
    sendByte(leds);
  }
}
void quayQuat(bool isQuay){
  if(isQuay)
    digitalWrite(FAN_PIN, HIGH);
  else
    digitalWrite(FAN_PIN, LOW);
}
void getDataDHT(float &temp, float &rh){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temp = event.temperature;
  rh = dht.humidity().getEvent(&event);
  rh = event.relative_humidity;
  // Serial.println(temp);
  // Serial.println(rh);
}
void wifiConnect(){
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connection established");
}
void send_data(){
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V2, relative_humidity);
}
byte toByte(String str) {
  int value = str.toInt();  // Works because `str` is of type `String`
  value = constrain(value, 0, 255);  // Optional: limit to 0–255
  return (byte)value;
}
void ledDoor(bool state){
  if(state){
    leds |= 1;
  }
  else{
    leds &= ~1;
  }
}
byte updateByteFromString(byte original, String str) {
  // Đảm bảo chuỗi có ít nhất 8 ký tự
  if (str.length() < 8) return original;

  for (int i = 0; i < 8; i++) {
    char c = str.charAt(i);

    if (c == '1') {
      original |= (1 << (7 - i));  // Bật bit
    } else if (c == '0') {
      original &= ~(1 << (7 - i)); // Tắt bit
    }
    // Nếu là ký tự khác (ví dụ 'x'), bỏ qua → giữ nguyên bit gốc
  }

  return original;
}
