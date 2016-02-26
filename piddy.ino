#include <OneWire.h>            // OneWire-Bibliothek einbinden
#include <DallasTemperature.h>  // DS18B20-Bibliothek einbinden
#include <LiquidCrystal.h>      // Lib for the LCD


#define DS18B20_PIN 12   // define the pin connected to the DS18B20s data wire
#define HEAT_RELAY_PIN 9 // define the pin connected to the heater relay
#define PUMP_IN_PIN 13// define the pin connected to the pump control button 
#define STEAM_IN_PIN 11// define the pin connected to the steam control button 
#define POTI_IN_PIN A1// define the pin connected to the temperature control poti 

#define PRINT_PLACES_AFTER_DECIMAL 1  // Number of decimals in printout
#define EMERGENCY_TEMP = 115  //max temperature before heater ist turned off hard

DeviceAddress   HEATER_SENSOR_ADDR = { 0x28, 0xAA, 0xAE, 0xC6, 0x06, 0x00, 0x00, 0xFD }; // address of the temperatur sensor connected to the heater;
DeviceAddress   WATER_SENSOR_ADDR = { 0x28, 0x58, 0x5C, 0xC7, 0x06, 0x00, 0x00, 0x4B };

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensors(&oneWire);   // DS18B20 initialisieren
LiquidCrystal lcd(3,4, 5, 6, 7, 8); // LCD initialisieren

/**
* GLOBAL VARIABLES
*/
int WINDOWSIZE = 500; // length of the relay circle in milliseconds
int heatStatus = 0; // status of the heater for printout only
unsigned long windowStartTime;  // starting point of the current heater cycle iteration
boolean pumping = false; //indicate if pump activ
boolean steaming = false; //indicate if steam selected

unsigned long temperaturePoti = 0; //Read value of the poti for the temperature
unsigned long lastLCDTime = 0; // most recent LCD update time

double currentTemperatureHeater = 0.0; // temperature of heater
double currentTemperatureWater = 0.0; // temperature of heater
double currentTemp =0.0; // agg Temperature

double weightHeater = 1.0; // weight for agg of temperature heater
double weightWater = 2.0; // weight for agg of temperature water

double pumpingHeat = 0.33;
double tolerance = 3.0;


//Define pid variables we'll be connecting to
double TargetTemp;  // the temperature that the water should get
double HeatTime; // the variable controling the heater


void setup()
{
  sensors.begin();  // init temp sensor
  setupHeater();
  setupSteam();
  setupPump();
  setupSerialInterface();
  setupLCD();
  setupPID();
  TargetTemp = 96.0;
}

void loop()
{
  updateTemp();
  if (istTempEmergency()) {
    return;
  }
  updateLCD();
  updateHeatTime();
  updatePump();
  updateSteam();
  updateHeater();
  updatePoti();
}

//
// Check Emergency Mode
// If Temperature is greater than
//
boolean istTempEmergency() {
  boolean emergency = sensors.getTempC(HEATER_SENSOR_ADDR) > 115;
  if (emergency) {
    turnHeatElementOnOff(0);
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
  pinMode(HEAT_RELAY_PIN , OUTPUT);
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
  TargetTemp = map(temperaturePoti,0,1024,80,100);
}


void updateHeatTime() {
  if(pumping){
    HeatTime = pumpingHeat*WINDOWSIZE;
  }else{
    if(currentTemperatureHeater > TargetTemp || currentTemperatureWater > TargetTemp ){
      HeatTime = 0;
      return;
    }
    double fullTemp = 0.5 * TargetTemp; 
    if(currentTemp < fullTemp){
      HeatTime = WINDOWSIZE;
    }
    double diffToHeat = TargetTemp - currentTemp;
    HeatTime = map(diffToHeat, 0 ,fullTemp, 0,WINDOWSIZE); 
 }
}

void updateTemp() {
  sensors.requestTemperatures();
  currentTemperatureHeater = sensors.getTempC(HEATER_SENSOR_ADDR);
  currentTemperatureWater = sensors.getTempC(WATER_SENSOR_ADDR);
  currentTemp = (weightHeater * currentTemperatureHeater + weightWater*currentTemperatureWater ) / (weightHeater + weightWater);
}

void updateLCD() {
  if (millis() - lastLCDTime > 1000) {
    lastLCDTime = millis();
    lcd.begin(16, 2);
    lcd.clear();
  }
  lcd.setCursor(0, 0);
  lcd.print(currentTemperatureHeater, PRINT_PLACES_AFTER_DECIMAL );
  lcd.print(" ");
  lcd.print(currentTemperatureHeater, PRINT_PLACES_AFTER_DECIMAL );
  lcd.print(" ");
  lcd.print(currentTemp,PRINT_PLACES_AFTER_DECIMAL);
  lcd.setCursor(0, 1);
  lcd.print(HeatTime, 1);
  lcd.print(" ");
  lcd.print(TargetTemp, 1);
}



void updateHeater() {
  boolean now = millis();
  if (now - windowStartTime > WINDOWSIZE) {
    windowStartTime = now;
  }
  if (HeatTime > now - windowStartTime) {
    turnHeatElementOnOff(1);
  } else {
    turnHeatElementOnOff(0);
  }
}

//
// HELPERS
//



void turnHeatElementOnOff(boolean on) {
  if (on) {
    heatStatus = 1;
    digitalWrite(HEAT_RELAY_PIN, HIGH);
  } else {
    heatStatus = 0 ;
    digitalWrite(HEAT_RELAY_PIN, LOW);
  }
}


