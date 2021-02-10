#include <FuzzyRule.h>
#include <FuzzyComposition.h>
#include <Fuzzy.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzyOutput.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzySet.h>
#include <FuzzyRuleAntecedent.h>


#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "0I479VBSx9O4IwJcURPtrKRsNARaKTRo";
char ssid[] = "Galaxy M21233A";
char pass[] = "ladakusajiku";

/* OLED */
//#include <ACROBOTIC_SSD1306.h> // library for OLED: SCL ==> D1; SDA ==> D2
#include <SPI.h>
#include <Wire.h>

//define Trig and Echo HC-SR04
#define trigpin D7
#define echopin D8
long duration;
int distance;
void sensorping();

/* RTC DS3231*/
#include "RTClib.h"
#include <LiquidCrystal_I2C.h> // Library for LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 RTC;
void getRTCdata();
//notif
#define BLYNK_MAX_SENDBYTES 256 //default 128

/* Soil Moister */
void getSoilMoisterData();
void getTempData();
void displayData();
#define soilMoisterPin A0
#define soilMoisterVcc D1
int soilMoister = 0;
int sensorValue;

/* DS18B0 Temp Sensor */
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 0 //D4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

//button Bylnk
BlynkTimer timer;
void checkPhysicalButton();
int relay1State = LOW;
int pushButton1State = HIGH;
#define VPIN_BUTTON_1    V12 
#define PUSH_BUTTON_1     D4   //D4

/*pump orange=VCC kuning=in2 green=in1 gnd=blue*/
#include <Adafruit_MotorShield.h>
//int relayInput = D5;
//int relayInputA = D6;
const int relay1 = D5; //pin2
const int relay2 = D6; //pin3
int relayON = LOW; //relay nyala
int relayOFF = HIGH; //relay mati
//Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
//Adafruit_DCMotor *relayInput = AFMS.getMotor(0);
//

int Humidity;
int temperature;
float output;
unsigned long timeNow;
unsigned long timeLast;

Fuzzy* fuzzy = new Fuzzy();

// Creating fuzzification of Temperature  
FuzzySet* dingin = new FuzzySet(0, 0, 20, 30);
FuzzySet* normal = new FuzzySet(25, 30, 30, 35); 
FuzzySet* panas = new FuzzySet(30, 35, 40, 40); 

// creating fuzzification of Humidity

FuzzySet* kering = new FuzzySet(0, 0, 237, 380);
FuzzySet* lembab = new FuzzySet(237, 380, 380, 712);
FuzzySet* basah = new FuzzySet(570, 712, 950, 950);

// Fuzzy output watering duration 

FuzzySet* cepat = new FuzzySet(0, 3, 3, 5);
FuzzySet* lumayan = new FuzzySet(3, 5, 5, 7);
FuzzySet* lama = new FuzzySet(5, 7, 7, 10);


void setup() 
{
  //Blynk debug console
  Blynk.begin(auth, ssid, pass); 
  timeNow = millis();
  
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  //AFMS.begin();
  sensor.begin();
  Serial.begin(115200);
//  dht.begin();
//  oledStart();
  digitalWrite (soilMoisterVcc, LOW);
  RTC.begin();
  lcd.begin();
  Wire.begin(4,5);
  lcd.backlight();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  Serial.print('Time and date set');
  lcd.setCursor(0, 0);
  lcd.print("loaaaaaaaaaaading");
  Serial.print("Real Time Clock");
  delay(500);
  lcd.clear();

  FuzzyInput* suhu = new FuzzyInput(1);

    suhu->addFuzzySet(dingin);
    suhu->addFuzzySet(normal); 
    suhu->addFuzzySet(panas);

    fuzzy->addFuzzyInput(suhu); 


    FuzzyInput* kelembapan = new FuzzyInput(2);

    kelembapan->addFuzzySet(kering);
    kelembapan->addFuzzySet(lembab);
    kelembapan->addFuzzySet(basah);

    fuzzy->addFuzzyInput(kelembapan);

    FuzzyOutput* WaktuPengairan = new FuzzyOutput(1);

    WaktuPengairan->addFuzzySet(cepat); 
    WaktuPengairan->addFuzzySet(lumayan); 
    WaktuPengairan->addFuzzySet(lama);
  
    fuzzy->addFuzzyOutput(WaktuPengairan);


    //fuzzy rule 
    
    FuzzyRuleConsequent* thenCepat = new FuzzyRuleConsequent();
    thenCepat->addOutput(cepat);
    FuzzyRuleConsequent* thenLumayan = new FuzzyRuleConsequent(); 
    thenLumayan->addOutput(lumayan);  
    FuzzyRuleConsequent* thenLama = new FuzzyRuleConsequent(); 
    thenLama->addOutput(lama);

    FuzzyRuleAntecedent* ifDinginDanBasah = new FuzzyRuleAntecedent();
    ifDinginDanBasah->joinWithAND(dingin, basah); 
    
    FuzzyRule* fuzzyRule01 = new FuzzyRule(1, ifDinginDanBasah, thenCepat);
    fuzzy->addFuzzyRule(fuzzyRule01); 


    FuzzyRuleAntecedent* ifDinginLembab = new FuzzyRuleAntecedent(); 
    ifDinginLembab->joinWithAND(dingin, lembab);
      
    FuzzyRule* fuzzyRule02 = new FuzzyRule(2, ifDinginLembab, thenCepat);
    fuzzy->addFuzzyRule(fuzzyRule02);

    FuzzyRuleAntecedent* ifDinginKering = new FuzzyRuleAntecedent(); 
    ifDinginKering->joinWithAND(dingin, kering);

  
    FuzzyRule* fuzzyRule03 = new FuzzyRule(3, ifDinginKering, thenLumayan);
    fuzzy->addFuzzyRule(fuzzyRule03); 

    FuzzyRuleAntecedent* ifNormalBasah = new FuzzyRuleAntecedent(); 
    ifNormalBasah->joinWithAND(normal, basah);

    FuzzyRule* fuzzyRule04 = new FuzzyRule(4, ifNormalBasah, thenCepat);
    fuzzy->addFuzzyRule(fuzzyRule04); 

    FuzzyRuleAntecedent* ifNormalLembab = new FuzzyRuleAntecedent(); 
    ifNormalLembab->joinWithAND(normal, lembab); 

    FuzzyRule* fuzzyRule05 = new FuzzyRule(5, ifNormalLembab, thenLumayan); 
    fuzzy->addFuzzyRule(fuzzyRule05); 

    FuzzyRuleAntecedent* ifNormalKering = new FuzzyRuleAntecedent(); 
    ifNormalKering->joinWithAND(normal, kering); 
 
    FuzzyRule* fuzzyRule06 = new FuzzyRule(6, ifNormalKering, thenLama); 
    fuzzy->addFuzzyRule(fuzzyRule06); 

    FuzzyRuleAntecedent* ifPanasBasah = new FuzzyRuleAntecedent(); 
    ifPanasBasah->joinWithAND(panas, basah); 

    FuzzyRule* fuzzyRule07 = new FuzzyRule(7, ifPanasBasah, thenLumayan); 
    fuzzy->addFuzzyRule(fuzzyRule07); 

    FuzzyRuleAntecedent* ifPanasLembab = new FuzzyRuleAntecedent(); 
    ifPanasLembab->joinWithAND(panas, lembab); 
      
    FuzzyRule* fuzzyRule08 = new FuzzyRule(8, ifPanasLembab, thenLumayan); 
    fuzzy->addFuzzyRule(fuzzyRule08);
  
    FuzzyRuleAntecedent* ifPanasKering = new FuzzyRuleAntecedent(); 
    ifPanasKering->joinWithAND(panas, kering); 

    FuzzyRule* fuzzyRule09 = new FuzzyRule(9, ifPanasKering, thenLama); 
    fuzzy->addFuzzyRule(fuzzyRule09);

}

void loop() 
{
  Blynk.run();
  timer.run();
//  getDhtData();
  getSoilMoisterData();
  getTempData();
  displayData();
  getRTCdata();
  DateTime now = RTC.now();
  int hum = sensorValue;
  int temp = temperature;
  int dur = ((output*1000)/1000);
  delay(2000); // delay for getting DHT22 data
 // Humidity = soilMoister.getCapacitance();

  
  fuzzy->setInput(1, temperature);
  fuzzy->setInput(2, soilMoister);

  fuzzy->fuzzify();
  output = fuzzy->defuzzify(1);

  //relayInput->run(FORWARD);
  if (sensorValue < 40 ){
  digitalWrite(relay1, relayON);
  Serial.print("pompa nyala selama : ");
  Serial.print(output);
  Serial.println(" ");
  delay(output*1000);
  //LCD blynk
  Blynk.virtualWrite(V9, (output*1000)/1000); //v5
  lcd.setCursor(0, 0);
  lcd.print("relay 01 nyala");
  lcd.setCursor(0, 1);
  lcd.print("durasi: ");
  lcd.print(dur);
  lcd.print(" S   ");
  delay(2000);
//  lcd.clear();
  

  digitalWrite(relay1, relayOFF);
  Serial.println("pump mati");
 // Blynk.virtualWrite(V5, relayInput); //v5
  }
  if (sensorValue > 40 ){
  
  //relayInput->run(RELEASE);
  digitalWrite(relay1, relayOFF);
  Serial.println("pump mati");
  //Blynk.virtualWrite(V5, relayInput); //v5
  //LCD//
  lcd.setCursor(0, 0);
  lcd.print("H: ");
  lcd.print(hum);
  lcd.print("%");
  lcd.print(" T: ");
  lcd.print(temp);
  lcd.print("'C");
  
 // lcd.setCursor(5, 1);
 // lcd.print(now.year(), DEC);
 // lcd.print(":");
 // lcd.print(now.month(), DEC);
 // lcd.print(":");
 // lcd.print(now.day(), DEC);
 
  lcd.setCursor(0, 1);
  lcd.print ("Time: "); 
  lcd.setCursor(6, 1);
  lcd.print(now.hour(), DEC);
  lcd.print(":");
  lcd.print(now.minute(), DEC);
  lcd.print(":");
  lcd.print(now.second(), DEC); 
  delay(2000);
  //lcd.clear();
  }
  timeLast = timeNow;
}


// When App button is pushed - switch the state

BLYNK_WRITE(VPIN_BUTTON_1) {
  relay1State = param.asInt();
  digitalWrite(relay2, relay1State);
  Serial.println("pump diaktifkan manual");
  Blynk.notify("pompa nyala {relay2} manual");
//    lcd.setCursor(0, 0);
//    lcd.print("relay 2 nyala");
//    lcd.setCursor(0, 1);
//    lcd.print("manual");
//    lcd.clear();
//    delay(1000);
}

void checkPhysicalButton()
{
  if (digitalRead(PUSH_BUTTON_1) == LOW) {
    // pushButton1State is used to avoid sequential toggles
    if (pushButton1State != LOW) {

      // Toggle Relay state
      relay1State = !relay1State;
      digitalWrite(relay2, relay1State);
      Serial.println("pump diaktifkan manual");
//        lcd.setCursor(0, 0);
//        lcd.print("relay 2 nyala");
//        lcd.setCursor(0, 1);
//        lcd.print("manual");
//        lcd.clear();
//        delay(1000);
      // Update Button Widget
      Blynk.virtualWrite(VPIN_BUTTON_1, relay1State);
      Serial.println("pump diaktifkan manual");
      Blynk.notify("pompa nyala {relay2} manual");
    }
    pushButton1State = LOW;
  } else {
    pushButton1State = HIGH;
  }
}

void getRTCdata(void)//tampil LCD
{
  DateTime now = RTC.now();
  int hum = sensorValue;
  int temp = temperature;

////  if ( relay1 = LOW)
////     {
////    lcd.setCursor(0, 0);
////    lcd.print("relay 1 nyala");
////    lcd.setCursor(0, 1);
////    lcd.print("durasi : ");
////    lcd.print(output*1000);
////    lcd.clear();
////    }
//  //if ( digitalWrite(relay1, HIGH))
// // {
//  lcd.setCursor(0, 0);
//  lcd.print("H: ");
//  lcd.print(hum);
//  lcd.print("%");
//  lcd.print(" T: ");
//  lcd.print(temp);
//  lcd.print("'C");
//  
// // lcd.setCursor(5, 1);
// // lcd.print(now.year(), DEC);
// // lcd.print(":");
// // lcd.print(now.month(), DEC);
// // lcd.print(":");
// // lcd.print(now.day(), DEC);
// 
//  lcd.setCursor(0, 1);
//  lcd.print ("Time: "); 
//  lcd.setCursor(6, 1);
//  lcd.print(now.hour(), DEC);
//  lcd.print(":");
//  lcd.print(now.minute(), DEC);
//  lcd.print(":");
//  lcd.print(now.second(), DEC); 
//  delay(2500);
//  lcd.clear();
// // }
}

/***************************************************
 * Start OLED
 **************************************************/
void oledStart(void)
{
  Wire.begin();  
}

void getTempData(void)
{
  //temprature
  sensor.requestTemperatures();                // Send the command to get temperatures  
  temperature = sensor.getTempCByIndex(0);
  delay(500);
  Blynk.virtualWrite(V11, temperature); //v11
  //
}

void sensorping ()
{
  digitalWrite(trigpin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigpin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigpin, LOW);
}
/***************************************************
 * Get DHT data
 **************************************************/


/***************************************************
 * Get Soil Moister Sensor data
 **************************************************/
void getSoilMoisterData(void)
{
  sensorValue = ( 100.00 - ( (analogRead(soilMoister) / 1023.00) * 100.00) );
  Blynk.virtualWrite(V10, sensorValue); //v11
}

/***************************************************
 * Display data at Serial Monitora & OLED Display
 **************************************************/
void displayData(void)
{
  Serial.println("==========================");
  //soil moister
  Serial.print("Kelembaban tanah :  ");
  Serial.print(sensorValue);
  Serial.println("%");
 
  //ping
  duration = pulseIn(echopin, HIGH);
  // Calculate the distance:
  distance = duration * 0.034 / 2;
  // Print the distance on the Serial Monitor (Ctrl+Shift+M):
  Serial.print("Jarak = ");
  Serial.print(distance);
  Serial.println(" cm");
  
  //RTC
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  //temp
  Serial.print("Temperatur:  ");
  Serial.print(sensor.getTempCByIndex(0));   // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  Serial.println(" ºC");
  Serial.println("==========================");
  delay(1000);
}

//void sendUptime()
//{
 
//}

BLYNK_CONNECTED() {

  // Request the latest state from the server

  Blynk.syncVirtual(VPIN_BUTTON_1);
   Blynk.virtualWrite(V11, temperature); //v11
 // Blynk.syncVirtual(VPIN_BUTTON_2);
 //  Blynk.syncVirtual(VPIN_BUTTON_3);
  //Blynk.syncVirtual(VPIN_BUTTON_4);

  // Alternatively, you could override server state using:
 // Blynk.virtualWrite(VPIN_BUTTON_1, relay1State);
 // Blynk.virtualWrite(VPIN_BUTTON_2, relay2State);
 // Blynk.virtualWrite(VPIN_BUTTON_3, relay3State);
 // Blynk.virtualWrite(VPIN_BUTTON_4, relay4State);

}
/***************************************************
 * Clear OLED Display
 **************************************************/
