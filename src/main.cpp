#include <Arduino.h>
#include <Keypad.h>


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
byte leds = 0;
// Function
void _keypad();
void sendByte(byte val);  
// void myShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t val);
// void updateShiftRegisterL();
// void updateShiftRegisterR();


void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  sendByte(0b11111100);
  sendByte(0b00111111);
}
void _keypad(){
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
  }
}
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
// void myShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t val) {
//   for (int i = 7; i >= 0; i--) {
//     digitalWrite(clockPin, LOW);
//     digitalWrite(dataPin, (val & (1 << i)) ? HIGH : LOW);
//     delayMicroseconds(1);       // thêm delay nhỏ
//     digitalWrite(clockPin, HIGH);
//     delayMicroseconds(1);
//   }
// }
// void updateShiftRegisterL(){
//   digitalWrite(latchPin, LOW);
//   shiftOut(dataPin, clockPin, MSBFIRST, leds);
//   digitalWrite(latchPin, HIGH);
// }
// void updateShiftRegisterR(){
//   digitalWrite(latchPin, LOW);
//   shiftOut(dataPin, clockPin, LSBFIRST, leds);
//   digitalWrite(latchPin, HIGH);
// }