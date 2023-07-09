#include <VoiceRecognitionV3.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <Keypad.h> 
#include <Servo.h> 

#define onRecord    (0)
#define offRecord   (1) 
#define Password_size 11 

VR myVR(11, 12);    // 2:RX 3:TX, you can choose your favourite pins.

uint8_t records[7]; // save record

uint8_t buf[64];

Servo myservo;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

int pos = 0; 
char Insert[Password_size]; 
char Unlocked[Password_size] = "3212322321";
byte data_count = 0, master_count = 0;
bool Pass_is_good;
char customKey;
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5}; 
byte colPins[COLS] = {6, 7, 8, 9}; 

bool door = true;

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

const int notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
const int buzzerPin = 13;

void printSignature(uint8_t *buf, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    if (buf[i] > 0x19 && buf[i] < 0x7F) {
      Serial.write(buf[i]);
    }
    else {
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

void printVR(uint8_t *buf)
{
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if (buf[0] == 0xFF) {
    Serial.print("NONE");
  }
  else if (buf[0] & 0x80) {
    Serial.print("UG ");
    Serial.print(buf[0] & (~0x80), DEC);
  }
  else {
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if (buf[3] > 0) {
    printSignature(buf + 4, buf[3]);
  }
  else {
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}

int findIndex(char key) {
  // Find the index of the pressed key in the hexaKeys array
  for (byte i = 0; i < ROWS; i++) {
    for (byte j = 0; j < COLS; j++) {
      if (hexaKeys[i][j] == key) {
        return i * COLS + j;
      }
    }
  }
  return -1;  // Key not found
}

void setup()
{ 
  myVR.begin(9600);
    if (myVR.clear() == 0) {
    Serial.println("Recognizer cleared.");
  }
  else {
    Serial.println("Not find VoiceRecognitionModule.");
    Serial.println("Please check connection and restart Arduino.");
    while (1);
  }

  if (myVR.load((uint8_t)onRecord) >= 0) {
    Serial.println("onRecord loaded");
  }

  if (myVR.load((uint8_t)offRecord) >= 0) {
    Serial.println("offRecord loaded");
  }

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  myservo.attach(10);
  ServoClose();
  pinMode(buzzerPin, OUTPUT);
}
void loop()
{
  if (door == 0)
  {
    customKey = customKeypad.getKey();
    int ret;
    ret = myVR.recognize(buf, 50);
    if(ret>0){
    switch(buf[1]){
      case offRecord:
      ServoClose();
      door = 1;
      break;
    }
    }
    if (customKey == '#')
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door is Closed");
      ServoClose();
      delay(3000);
      door = 1;
    }
  }
  else
  {
    Open();
  }
}

void clearData()
{
  while (data_count != 0)
  {
    Insert[data_count--] = 0;
  }
  return;
}

void ServoOpen()
{
  myservo.write(0);
}

void ServoClose()
{
  myservo.write(180);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
}

void Open()
{
  
  customKey = customKeypad.getKey();
   int ret;
  ret = myVR.recognize(buf, 50);
  if(ret>0){
    switch(buf[1]){
      case onRecord:
      ServoOpen();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door is Open");
      door = 0;
      break;
    }
    /** voice recognized */
    printVR(buf);
  }
  if (customKey) {
    Insert[data_count] = customKey;
    lcd.setCursor(data_count, 1);
    lcd.print("*");
    int index = findIndex(customKey);
    if (index >= 0) {
      int frequency = notes[index];
      tone(buzzerPin, frequency);
      delay(200);
      noTone(buzzerPin);
    }

    data_count++;

    if (data_count == Password_size - 1) {
      lcd.clear();
      if (!strcmp(Insert, Unlocked)) {
        ServoOpen();
        lcd.setCursor(0, 0);
        lcd.print("Door is Open");
        door = 0;
      }
      else {
        lcd.setCursor(0, 0);
        lcd.print("Wrong Password");
        delay(1000);
        door = 1;
      }
      clearData();
    }
  }
}


