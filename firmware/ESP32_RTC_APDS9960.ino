// HEADER FILES
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>
#include <SparkFun_APDS9960.h> 

// OBJECT
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
BluetoothSerial SerialBT;
SparkFun_APDS9960 apds; 

// DEFINE
int buzzerPin = 19;
int alarmHour = -1;
int alarmMinute = -1;
bool alarmSet = false;
bool alarmTriggered = false;

// SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 
  lcd.begin(16, 2);
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  SerialBT.begin("GESTURE ALARM CLOCK");
  Serial.println("SET ALARM HH:MM");

  if (!rtc.begin()) {
    lcd.print("RTC not found!");
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (apds.init()) {
    Serial.println("APDS9960 OK");
    apds.enableGestureSensor(true);
  } else {
    Serial.println("APDS9960 Error!");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Gesture Clock");
  lcd.setCursor(0, 1);
  lcd.print("Alarm Clock ");
  delay(2000);
  lcd.clear();
}

// LOOP
void loop() {
  DateTime now = rtc.now();

  // Display time 
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printDigits(now.hour());
  lcd.print(":");
  printDigits(now.minute());
  lcd.print(":");
  printDigits(now.second());

  lcd.setCursor(0, 1);
  if (alarmSet) {
    lcd.print("Alarm: ");
    printDigits(alarmHour);
    lcd.print(":");
    printDigits(alarmMinute);
  } else {
    lcd.print("No Alarm Set   ");
  }

  if (alarmSet && !alarmTriggered) {
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
    triggerAlarm();
    }
  }

  if (SerialBT.available()) {
    String input = SerialBT.readStringUntil('\n');
    input.trim();
    Serial.println("Received: " + input);
    processBluetoothCommand(input);
  }
  delay(1000);
}

// LCD
void printDigits(int digits) {
  if (digits < 10) lcd.print("0");
  lcd.print(digits);
}

// ALARM TRIGGER
void triggerAlarm() {
  alarmTriggered = true;
  lcd.clear();
  lcd.print("ALARM! WAKE UP!");
  SerialBT.println("Alarm Triggered!");
  Serial.println("Alarm Triggered! Wave to stop.");

  unsigned long startTime = millis();
  bool gestureDetected = false;

  while (millis() - startTime < 30000) {
    digitalWrite(buzzerPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
    delay(300);
    if (apds.isGestureAvailable()) {
      int gesture = apds.readGesture();
      if (gesture == DIR_RIGHT || gesture == DIR_LEFT || gesture == DIR_UP || gesture == DIR_DOWN) {
        gestureDetected = true;
        break;
      }
    }
  }

  digitalWrite(buzzerPin, LOW);
  lcd.clear();

  if (gestureDetected) {
    lcd.print("Gesture Stopped");
    SerialBT.println("Alarm stopped by gesture");
  } else {
    lcd.print("Automatically stopped");
    SerialBT.println("Alarm stopped (timeout)");
  }

  delay(2000);
  lcd.clear();
  
  alarmTriggered = false;
  alarmSet = false;
}

// BLUETOOTH COMMUNICATION
void processBluetoothCommand(String input) {

  input.toUpperCase();
  if (input.startsWith("ALARM")) {
    int spaceIndex = input.indexOf(' ');
  if (spaceIndex > 0) {
      String timePart = input.substring(spaceIndex + 1);
      int colonIndex = timePart.indexOf(':');

  if (colonIndex > 0) {
      alarmHour = timePart.substring(0, colonIndex).toInt();
      alarmMinute = timePart.substring(colonIndex + 1).toInt();
      alarmSet = true;
      alarmTriggered = false;

      SerialBT.println("Alarm set for " + String(alarmHour) + ":" + String(alarmMinute));
      lcd.clear();
      lcd.print("Alarm Set: ");
      printDigits(alarmHour);
      lcd.print(":");
      printDigits(alarmMinute);
      delay(2000);
      lcd.clear();
      }
    }
  } 
  else if (input == "STOP") {
    digitalWrite(buzzerPin, LOW);
    alarmTriggered = false;
    alarmSet = false;
    lcd.clear();
    lcd.print("Alarm stopped");
    SerialBT.println("Alarm stopped manually");
    delay(1500);
    lcd.clear();
  } 
  else {
    SerialBT.println("Invalid Command");
  }
}
