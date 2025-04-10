#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BMP280.h>
#include <NewPing.h>
#include <MQUnifiedsensor.h>


#define MAX_SLURRY_CM 8
#define MIN_PH 0
#define MAX_PH 14
#define echo 35
#define trig 32
#define PH_PIN A15


//Definitions
#define placa "ESP32"
#define Voltage_Resolution 5
#define pin A18 //Analog input 4 of your arduino
#define type "MQ-8" //MQ4
#define ADC_Bit_Resolution 12 // For arduino UNO/MEGA/NANO
#define RatioMQ4CleanAir 4.4  //RS / R0 = 4.4 ppm 
//#define calibration_button 13 //Pin to calibrate your sensor

WiFiClient espClient;
PubSubClient client(espClient);


// put function declarations here:
Adafruit_BMP280 bmp;

MQUnifiedsensor MQ8(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

NewPing sonar(trig , echo , 400);

void setup() {

  Serial.begin(9600);

  MQ8.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ8.setA(1012.7); 
  MQ8.setB(-2.786);

  MQ8.init(); 

  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ8.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ8.calibrate(RatioMQ4CleanAir);
    Serial.print(".");
  }
  MQ8.setR0(calcR0/10);
  Serial.println("  done!.");

  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}

  MQ8.serialDebug(true);


  while (!bmp.begin(0x76))
  {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    delay(1000);
  }
}

void loop() {

  MQ8.update();
  float methanePPM = MQ8.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  float external_temperature = bmp.readTemperature();
  float altitude = bmp.readAltitude(1018.4);

  int cm = sonar.ping_cm();
  float slurrly_level = MAX_SLURRY_CM * 100 / cm;

  int rawPh = analogRead(PH_PIN);
  float voltage = rawPh * (5 / 4095.0); 
  float ph = 7.0 - ((2.5 - voltage) / 0.18);

  Serial.println(ph);


  delay(500);
}


// String toJson(float avg_temperature , float humidity, float pressure, float soil_moisture , float aprox_altitude){
//   JsonDocument doc;
//   // doc["temperature"] = ceil(avg_temperature * 100.0) / 100.0;
//   // doc["humidity"] = ceil(humidity * 100.0) /100.0;
//   // doc["pressure"] = ceil(pressure * 100.0) /100.0 ;
//   // doc["soil moisture"] = soil_moisture;
//   // doc["heat index"] = ceil(dht.computeHeatIndex(avg_temperature, humidity,false) * 100.0) / 100.0;
//   // doc["altitude"] = ceil(aprox_altitude * 100.0) / 100.0;
//   // String output;
//   // serializeJson(doc, output);
//   // return output;
// }