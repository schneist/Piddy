#include <OneWire.h>            // OneWire-Bibliothek einbinden
#include <DallasTemperature.h>  // DS18B20-Bibliothek einbinden


#define DS18B20_PIN 12   // define the pin connected to the DS18B20s data wire
#define CAULDRON_RELAY_PIN 9 // define the pin connected to the cauldron heater relay
#define STEAM_IN_PIN 11// define the pin connected to the steam control button 

#define RED_PIN 3
#define GREEN_PIN 5
#define BLUE_PIN 6

#define PRINT_PLACES_AFTER_DECIMAL 1  // Number of decimals in printout

long WINDOWSIZE = 2000; // length of the relay circle in milliseconds


DeviceAddress   BOILER_SENSOR_ADDR = { 0x28, 0x61, 0x64, 0x11, 0xBD, 0x8B, 0x7C, 0xA8 }; // address of the temperature sensor connected to the cauldron

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensors(&oneWire);   // DS18B20 initialisieren


/**
  GLOBAL VARIABLES
*/
unsigned long windowStartTime = 0;  // starting point of the current heater cycle iteration
boolean steaming = false; //indicate if steam selected
boolean heating = false; //indicate if steam selected

double currentTemperatureCauldron = 0.0; // temperature of heater
double TargetTempCoffee = 87.0; // the temperature that the water should get
double TargetTempSteam = 95.0; // the temperature that the water should get
double TargetTemp = TargetTempCoffee; // the temperature that the water should get

double HeatTimeCauldron; // the variable controling the heater
double diffToHeat = 0;


int hueRed = 0;
int hueGreen = 1000;
int hueBlue = 1000;


void setup() {
  sensors.begin();  // init temp sensor
  sensors.setResolution(BOILER_SENSOR_ADDR, 10);
  setupHeater();
  setupSteam();
  //setupSerialInterface();
  setupPID();
  setupLED();
}


void loop()
{
  switchCauldronHeater();
  updateSteam();
  updateLED();
 
}



//
// SETUPS
//


void setupLED(){
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}


void setupSteam() {
  pinMode(STEAM_IN_PIN , INPUT);
}

void setupHeater() {
  pinMode(CAULDRON_RELAY_PIN , OUTPUT);
}

void setupSerialInterface() {
  Serial.begin(19200);
}

void setupPID() {
  windowStartTime = millis();
}

//
// UPDATES
//

void updateLED(){
  if(abs(currentTemperatureCauldron-TargetTemp)>=1.33){
      hueRed = 0;
      hueGreen = 1000;
      hueBlue = 1000; 
  }else{
      hueRed = 1000;
      hueGreen = 0;
      hueBlue = 1000;
  }
  digitalWrite(RED_PIN, hueRed);
  digitalWrite(GREEN_PIN, hueGreen);
  digitalWrite(BLUE_PIN, hueBlue);
}


void updateSteam() {
  steaming = digitalRead(STEAM_IN_PIN);
  if(steaming){
    TargetTemp = TargetTempSteam;
  }else{
    TargetTemp = TargetTempCoffee;
  }
}


void updateLCD() {
    Serial.println(currentTemperatureCauldron, PRINT_PLACES_AFTER_DECIMAL);
    Serial.println(diffToHeat);
}

void switchCauldronHeater() {
  unsigned long now = millis();
  if ((now - windowStartTime) >= WINDOWSIZE) {
    updateTemp();
    updateHeatTimeCauldron();
    //updateLCD();
    now = millis();
    windowStartTime = now;
    
  }
  if (HeatTimeCauldron > (now - windowStartTime)) {
    heating = true;
    digitalWrite(CAULDRON_RELAY_PIN, HIGH);
  } else {
    heating = false;
    digitalWrite(CAULDRON_RELAY_PIN, LOW);
  }
}


void updateHeatTimeCauldron() {
  double diff = (TargetTemp - currentTemperatureCauldron) / 15.0;
  diffToHeat = (exp(diff) - 1) ;
  HeatTimeCauldron = diffToHeat * WINDOWSIZE;
}


void updateTemp() {
  sensors.requestTemperatures();
  currentTemperatureCauldron  = sensors.getTempC(BOILER_SENSOR_ADDR);
}
