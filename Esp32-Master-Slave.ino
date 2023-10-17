#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// To declare I2C communication between lcd and esp32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_BME280 bme;

// should be changed depending on the MAC Address of your receiver 
uint8_t broadcastAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Define variables to store BME280 readings to be sent
float temper;
float humid;
float pres;

// Define variables to store incoming readings
float receivedtemp;
float receivedHum;
float receivedPres;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure


typedef struct struct_message {
    float temp;
    float hum;
    float press;
} struct_message;

// Create a struct_message called BME280Readings to hold sensor readings
struct_message BMEReadings;

// Create a struct_message to hold incoming sensor readings
struct_message receivedReadings;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedReadings, incomingData, sizeof(receivedReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = receivedReadings.temp;
  incomingHum = receivedReadings.hum;
  incomingPress = receivedReadings.press;
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Init BME280 sensor
  bool status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor");
    while (1);
  }

  // Init OLED display
  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
{ 
    Serial.println(F("SSD1306 allocation failed"));
    
  }
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  getReadings();
 
  // Set values to send
  BMEReadings.temp = temper;
  BMEReadings.hum = humid;
  BMEReadings.press = pres;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &BMEReadings, sizeof(BMEReadings));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  updateDisplay();
  delay(10000);
}
void getReadings(){
  temper = bme.readTemperature();
  humid = bme.readHumidity();
  pres = (bme.readPressure() / 100.0F);
}

void updateDisplay(){
  // Display Readings on OLED Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("INCOMING READINGS");
  display.setCursor(0, 15);
  display.print("Temperature: ");
  display.print(incomingTemp);
  display.cp437(true);
  display.write(248);
  display.print("C");
  display.setCursor(0, 25);
  display.print("Humidity: ");
  display.print(incomingHum);
  display.print("%");
  display.setCursor(0, 35);
  display.print("Pressure: ");
  display.print( incomingPress);
  display.print("hPa");
  display.setCursor(0, 56);
  display.print(success);
  display.display();
  
  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("Temperature: ");
  Serial.print(receivedReadings.temp);
  Serial.println(" ºC");
  Serial.print("Humidity: ");
  Serial.print(receivedReadings.hum);
  Serial.println(" %");
  Serial.print("Pressure: ");
  Serial.print(receivedReadings.press);
  Serial.println(" hPa");
  Serial.println();
}