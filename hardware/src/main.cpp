#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BMP280.h>
#include <NewPing.h>
#include <MQUnifiedsensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>


#define MAX_SLURRY_CM 8
#define MIN_PH 0
#define MAX_PH 14
#define echo 35
#define trig 32
#define PH_PIN A15

#define TFT_DC 17
#define TFT_CS 26
#define TFT_RST 27
#define TFT_CLK 18
#define TFT_MOSI 13
#define TFT_MISO 14


//Definitions
#define placa "ESP32"
#define Voltage_Resolution 5
#define ONE_WIRE_BUS 4
#define pin A18 //Analog input 4 of your arduino
#define type "MQ-8" //MQ4
#define ADC_Bit_Resolution 12 // For arduino UNO/MEGA/NANO
#define RatioMQ4CleanAir 4.4  //RS / R0 = 4.4 ppm 
//#define calibration_button 13 //Pin to calibrate your sensor

// Add to the top section with other function declarations
void drawParameterBox(int x, int y, const char* label, float value, const char* unit, uint16_t color);
void display(float, float, float , float , float, float);

WiFiClient espClient;
PubSubClient client(espClient);

int count = 0;

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

// put function declarations here:
Adafruit_BMP280 bmp;

MQUnifiedsensor MQ8(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NewPing sonar(trig , echo , 400);

const char *ssid = "MonaConnect";
const char *password = "";

//MQTT client
const char *broker = "broker.emqx.io";
const int port = 1883;

void setup() {
  
  Serial.begin(9600);

  MQ8.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ8.setA(1012.7); 
  MQ8.setB(-2.786);

  MQ8.init(); 
  sensors.begin();

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


  tft.begin();
  // tft.setRotation(3);
  tft.fillScreen(0x000F55);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(30,30);
 
  delay(100);
}

void loop() {

  MQ8.update();
  float methanePPM = MQ8.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  float external_temperature = bmp.readTemperature();
  float altitude = bmp.readAltitude(1018.4);

  int cm = sonar.ping_cm();
  float slurry_level = 100 - ((cm == 0? 1:cm) * 100 / 8.0 );

  int rawPh = analogRead(PH_PIN);
  float voltage = rawPh * (5 / 4095.0); 
  float ph = 7.0 - ((2.5 - voltage) / 0.18);

  sensors.requestTemperatures(); 
  
  float internal_temp = sensors.getTempCByIndex(0);

  display(methanePPM, external_temperature, slurry_level, altitude, ph, internal_temp);
  
  delay(2000);


}

void display(float methanePPM, float external_temperature, float slurry_level, float altitude, float ph, float internal_temp) {
  // Dark blue background for professional look
  
  // Title bar
  tft.fillRect(0, 0, 320, 20, 0x05A5);  // Teal header
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(90, 6);
  tft.print("BIODIGESTER MONITOR");
  
  // Parameter boxes with rounded corners and borders
  drawParameterBox(10, 25, "METHANE", methanePPM, "PPM", 0xF800); // Red for methane (danger)
  drawParameterBox(10, 70, "EXT TEMP", external_temperature, "C", 0xFBE0); // Yellow-orange for external temp
  drawParameterBox(10, 115, "SLURRY LEVEL", slurry_level, "%", 0x07FF); // Cyan
  drawParameterBox(10, 160, "pH LEVEL", ph, "", 0x07E0); // Green
  
  // Bottom row with two smaller boxes for altitude and internal temp
  drawParameterBox(10, 205, "ALTITUDE", altitude, "m", 0xB5B6); // Light purple for altitude
  drawParameterBox(165, 205, "INT TEMP", internal_temp, "C", 0xF81F); // Magenta for internal temp
}

// Helper function to draw a nice parameter box
void drawParameterBox(int x, int y, const char* label, float value, const char* unit, uint16_t color) {
  int width = (strcmp(label, "ALTITUDE") == 0 || strcmp(label, "INT TEMP") == 0) ? 145 : 300;
  
  // Draw rounded rectangle with border
  tft.fillRoundRect(x, y, width, 40, 5, 0x4228); // Dark gray box
  tft.fillRoundRect(x+2, y+2, width-4, 36, 5, 0x2104); // Lighter inside for 3D effect
  
  // Draw label
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(x+8, y+6);
  tft.print(label);
  
  // Draw value with larger text
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setCursor(x+10, y+18);
  
  // Format value with proper precision
  if (strcmp(label, "pH LEVEL") == 0) {
    tft.print(value, 2); // pH with 2 decimal places
  } else {
    tft.print(value, 1); // Others with 1 decimal place
  }
  
  // Draw units
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  int unitX = (strcmp(label, "ALTITUDE") == 0 || strcmp(label, "INT TEMP") == 0) ? x+80 : x+100;
  tft.setCursor(unitX, y+25);
  tft.print(unit);
}
// void init_connections(){
//   Serial.println("Startinf");
//   WiFi.begin(ssid,password);
//   int retryCount = 0;
//   int print = 0;
//   while(!WiFi.isConnected() && retryCount < 10){
//     if(print==0){
//       tft.setTextSize(2);
//       print=1;
//       tft.printf("Connecting to wifi network %s", ssid);
//     }
//     Serial.printf("Connecting to wifi network %s\n",ssid);
//     vTaskDelay(1000 / portTICK_PERIOD_MS); // Use vTaskDelay instead of delay
//     retryCount++;
//   }
//   if(WiFi.isConnected()){
//     Serial.println("Connected to wifi nework");
//   }
//   client.setServer(broker,port);
  
//   retryCount = 0;
//   while(!client.connect("test1") && retryCount < 10){
//     Serial.println("connecting");
//     vTaskDelay(1000 / portTICK_PERIOD_MS); 
//     retryCount++;
//   }
// }

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