
//################# LIBRARIES ##########################
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <ArduinoJson.h>
//#include <TaskScheduler.h>
#include <SPI.h>
#include <MFRC522.h>
#include "configs.h"



// ################## Configurations and constants ###################
// rename configs -sample.h to configs.h and change parameters  

//RFiD Reader settings
#define RST_PIN         D3         // Configurable, see typical pin layout above
#define SS_PIN          D8        // Configurable, see typical pin layout above

#define WiFi_led        D4
#define indicator_pin   D1

const int toneFreq = 300;
const int toneDelay = 50;
const int toneRepeat = 3;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

//MFRC522::MIFARE_Key key;

//HTTP Integration
String jsonData, content = "";
DynamicJsonDocument doc(1024);

// We create the Scheduler that will be in charge of managing the tasks
//Scheduler runner;

// We create the task indicating that it runs every 500 milliseconds, forever, and call the led_blink function
//int taskDuration = 60000;
// Task TareaLED(taskDuration, TASK_FOREVER, &httpRequest);

//#################### Functions declerations  #####################
// We declare the function that we are going to use
// void httpRequest();

void setup()
{
  pinMode(WiFi_led, OUTPUT);
  pinMode(indicator_pin, OUTPUT);
  // setup rfid reader
  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus

  rfid.PCD_Init(); // Init MFRC522

  //  for (byte i = 0; i < 6; i++) {
  //    key.keyByte[i] = 0xFF;
  //  }
  // setup Wifi connection
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.println("Connecting...");
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(200);
    Serial.print('.');
    digitalWrite(WiFi_led, HIGH);
    delay(200);
    digitalWrite(WiFi_led, LOW);
  }

  // WiFi connection logging
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  doc["machine_id"] = WiFi.macAddress();
  digitalWrite(WiFi_led, HIGH);


}

void loop() {

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  jsonData = content = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  doc["RFID"] = content;
  serializeJson(doc, jsonData);
  Serial.println("UID: " + jsonData);
  httpGETRequest(serverName, jsonData);
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

// ############ Functions ##########

String httpGETRequest(String serverName, String jsonData) {
  WiFiClientSecure client;
  client.setInsecure(); //the magic line, use with caution
  client.connect(serverName, 443);
  HTTPClient http;
  http.begin(client, serverName);

  //  http.addHeader("Authorization", "Bearer xoxp-xxxxxx", true);
  http.addHeader("Content-Type", "application/json");


  // Send HTTP POST request
  int httpResponseCode = http.POST(jsonData);

  String payload = "{}";

  if (httpResponseCode == 200 || httpResponseCode == 201 ) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println(payload);
    playTone(indicator_pin, toneFreq, toneDelay, toneRepeat);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void playTone(int pin, int toneFreq, int toneDelay, int toneRepeat ) {
  for (int i = 0; i < toneRepeat; i++) {
    tone(pin, toneFreq);
    delay(toneDelay);
    noTone(indicator_pin);
    delay(toneDelay);

  }
}
