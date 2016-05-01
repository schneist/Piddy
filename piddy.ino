#include <OneWire.h>            // OneWire-Bibliothek einbinden
#include <DallasTemperature.h>  // DS18B20-Bibliothek einbinden
#include <LiquidCrystal.h>      // Lib for the LCD

#define DS18B20_PIN 12   // define the pin connected to the DS18B20s data wire
#define CAULDRON_RELAY_PIN 9 // define the pin connected to the cauldron heater relay
#define PRE_HEAT_RELAY_PIN 10 // define the pin connected to the preheater relay
#define PUMP_IN_PIN 13// define the pin connected to the pump control button 
#define STEAM_IN_PIN 11// define the pin connected to the steam control button 
#define POTI_IN_PIN A1// define the pin connected to the temperature control poti 

#define PRINT_PLACES_AFTER_DECIMAL 1  // Number of decimals in printout


long EMERGENCY_TEMP = 115;  //max temperature before heater ist turned off hard
long WINDOWSIZE = 1000; // length of the relay circle in milliseconds


DeviceAddress   PRE_HEATER_SENSOR_ADDR = { 0x28, 0x7F, 0x52, 0xC7, 0x06, 0x00, 0x00, 0xDA };  // address of the temperatur sensor connected to the preheater;
DeviceAddress   BOILER_SENSOR_ADDR = { 0x28, 0xAA, 0xAE, 0xC6, 0x06, 0x00, 0x00, 0xFD }; // address of the temperature sensor connected to the cauldron

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensors(&oneWire);   // DS18B20 initialisieren
LiquidCrystal lcd(3, 4, 5, 6, 7, 8); // LCD initialisieren

/**
  GLOBAL VARIABLES
*/
unsigned long windowStartTime = 0;  // starting point of the current heater cycle iteration
boolean pumping = false; //indicate if pump activ
boolean steaming = false; //indicate if steam selected

unsigned long temperaturePoti = 0; //Read value of the poti for the temperature
unsigned long lastLCDTime = 0; // most recent LCD update time

double currentTemperaturePreHeater = 0.0; // temperature of heater
double currentTemperatureCauldron = 0.0; // temperature of heater
double TargetTemp;  // the temperature that the water should get
double HeatTimeCauldron; // the variable controling the heater
double HeatTimePreHeater; // the variable controling the heater


void setup() {
  sensors.begin();  // init temp sensor
  setupHeater();
  setupSteam();
  setupPump();
  setupSerialInterface();
  setupLCD();
  setupPID();
}

void loop()
{
  updateTemp();
  if (istTempEmergency()) {
    return;
  }
  updateHeatTimePre();
  updateHeatTimeCauldron();
  switchPreHeater();
  switchCauldronHeater();
  updateLCD();
  updatePump();
  updateSteam();
  updatePoti();
}


boolean istTempEmergency() {

  boolean emergency = ( (currentTemperaturePreHeater > EMERGENCY_TEMP )  || (  currentTemperatureCauldron > EMERGENCY_TEMP ));
  if (emergency) {
    digitalWrite(PRE_HEAT_RELAY_PIN, LOW);
    digitalWrite(CAULDRON_RELAY_PIN, LOW);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HEAT ERROR!!");
    lcd.setCursor(0, 1);
    lcd.print("TURN OFF !!");
  }
  return emergency;
}

//
// SETUPS
//
void setupPump() {
  pinMode(PUMP_IN_PIN , INPUT);
}

void setupSteam() {
  pinMode(STEAM_IN_PIN , INPUT);
}

void setupHeater() {
  pinMode(CAULDRON_RELAY_PIN , OUTPUT);
  pinMode(PRE_HEAT_RELAY_PIN , OUTPUT);

}

void setupSerialInterface() {
  Serial.begin(9600);
}

void setupPID() {
  windowStartTime = millis();
}

void setupLCD() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("NOT YET READY !");
}
//
// UPDATES
//
void updatePump() {
  pumping = digitalRead(PUMP_IN_PIN);
}

void updateSteam() {
  steaming = !digitalRead(STEAM_IN_PIN);
}

void updatePoti() {
  temperaturePoti = analogRead(POTI_IN_PIN);
  TargetTemp = map(temperaturePoti, 0, 1024, 80, 100);
}


void updateHeatTimePre() {
  double diff = (TargetTemp - currentTemperaturePreHeater) / 15.0;
  double diffToHeat = (exp(diff) - 1) / 6;
  HeatTimePreHeater = diffToHeat * WINDOWSIZE;
}


void switchPreHeater() {
  unsigned long now = millis();
  if ((now - windowStartTime) >= WINDOWSIZE) {
    windowStartTime = now;
  }
  if (HeatTimePreHeater > (now - windowStartTime)) {
    digitalWrite(PRE_HEAT_RELAY_PIN, HIGH);
  } else {
    digitalWrite(PRE_HEAT_RELAY_PIN, LOW);
  }
}


void switchCauldronHeater() {
  unsigned long now = millis();
  if ((now - windowStartTime) >= WINDOWSIZE) {
    windowStartTime = now;
  }
  if (HeatTimeCauldron > (now - windowStartTime)) {
    digitalWrite(CAULDRON_RELAY_PIN, HIGH);
  } else {
    digitalWrite(CAULDRON_RELAY_PIN, LOW);
  }
}


void updateHeatTimeCauldron() {
  double diff = (TargetTemp - currentTemperatureCauldron) / 15.0;
  double diffToHeat = (exp(diff) - 1) / 6;
  HeatTimeCauldron = diffToHeat * WINDOWSIZE;
}


void updateTemp() {
  sensors.requestTemperatures();
  currentTemperaturePreHeater = sensors.getTempC(PRE_HEATER_SENSOR_ADDR);
  currentTemperatureCauldron  = sensors.getTempC(BOILER_SENSOR_ADDR);
}

void updateLCD() {
  long now = millis();
  if (now - lastLCDTime > 1000) {
    lastLCDTime = now;
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(TargetTemp, PRINT_PLACES_AFTER_DECIMAL);
    lcd.print(" ");
    lcd.print(currentTemperaturePreHeater, PRINT_PLACES_AFTER_DECIMAL);
    lcd.print(" ");
    lcd.print(currentTemperatureCauldron, PRINT_PLACES_AFTER_DECIMAL);
    lcd.setCursor(0, 1);
    lcd.print(HeatTimePreHeater, 0);
    lcd.print(" ");
    lcd.print(HeatTimeCauldron, 0);
    lcd.print(" ");
  }
}



