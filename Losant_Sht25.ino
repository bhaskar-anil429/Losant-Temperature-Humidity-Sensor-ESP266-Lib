/**
 * Example for sending temperature and humidity
 * to the cloud using the SHT25 and ESP8266
 *
 * Copyright (c) 2016 Losant IoT. All rights reserved.
 * https://www.losant.com
 * hardware 
 * https://store.ncd.io/product/esp8266-iot-communication-module-with-integrated-usb/
 * https://store.ncd.io/product/sht25-humidity-and-temperature-sensor-%C2%B11-8rh-%C2%B10-2c-i2c-mini-module/
 * https://store.ncd.io/product/i2c-shield-for-particle-electron-or-particle-photon-with-outward-facing-5v-i2c-port/
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Losant.h>
#include <Wire.h>

#define Addr 0x40 /// i2c address of SHT25
float humidity_sht25;
float cTemp;
float fTemp;
const char* ssid = "NETGEAR34";
const char* password = "sillyviolet195";

//const char* LOSANT_DEVICE_ID = "5a6f8f6a02a2340007b19540";
//const char* LOSANT_ACCESS_KEY = "f6b0b4c8-2c18-4112-b1d2-e3db60daff8c";
//const char* LOSANT_ACCESS_SECRET = "53831c433079911cc372d1a51ee82d199899a2dbf1f6cdb1663f4c169f8a513b";

const char* LOSANT_DEVICE_ID = "5a7090ce70b48a000767d599";
const char* LOSANT_ACCESS_KEY = "55211075-8511-4c98-bfb1-91db02be7fcf";
const char* LOSANT_ACCESS_SECRET = "d29f22631a9d7e7ac07c9fee0bc982139c9579c70657ba0f5480bedf5d879acc";
WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(500); 
  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);
      Serial.println();
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if(millis() - wifiConnectStart > 5000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.print("Authenticating Device...");
  HTTPClient http;
  http.begin("http://api.losant.com/auth/device");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = LOSANT_DEVICE_ID;
  root["key"] = LOSANT_ACCESS_KEY;
  root["secret"] = LOSANT_ACCESS_SECRET;
  String buffer;
  root.printTo(buffer);

  int httpCode = http.POST(buffer);

  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
          Serial.println("This device is authorized!");
      } else {
        Serial.println("Failed to authorize device to Losant.");
        if(httpCode == 400) {
          Serial.println("Validation error: The device ID, access key, or access secret is not in the proper format.");
        } else if(httpCode == 401) {
          Serial.println("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.");
        } else {
           Serial.println("Unknown response from API");
        }
      }
    } else {
        Serial.println("Failed to connect to Losant API.");

   }

  http.end();

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.println();
  Serial.println("This device is now ready for use!");
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  Wire.begin(12, 14);
  // Wait for serial to initialize.
  while(!Serial) { }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running SHT25!");
  Serial.println("-------------------------------------");

  connect();
}
void report(double humidity, double temp, double tempF) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["humidity"] = humidity;
  root["temp"] = cTemp;
  root["tempF"] = fTemp;
  device.sendState(root);
  Serial.println("Reported!");
}

int timeSinceLastRead = 0;
void loop() {
   bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  device.loop();

//////////////i2c starts over here /////////////////
  // Report every 2 seconds.

  unsigned int data[2];
  // Start I2C transmission
  Wire.beginTransmission(Addr);
  // Send humidity measurement command, NO HOLD master
  Wire.write(0xF5);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(500);

  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);

  // Read 2 bytes of data
  // humidity msb, humidity lsb
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data
    humidity_sht25 = (((data[0] * 256.0 + data[1]) * 125.0) / 65536.0) - 6;
  }

  // Start I2C transmission
  Wire.beginTransmission(Addr);
  // Send temperature measurement command, NO HOLD master
  Wire.write(0xF3);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(500);

  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);

  // Read 2 bytes of data
  // temp msb, temp lsb
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data
    cTemp = (((data[0] * 256.0 + data[1]) * 175.72) / 65536.0) - 46.85;
    fTemp = (cTemp * 1.8) + 32;
  }
//////////////i2c ends over here /////////////////

  if(timeSinceLastRead > 2000) {
    // Reading temperature or humidity takes about 250 milliseconds!
    float h = humidity_sht25;
    // Read temperature as Celsius (the default)
    float t = cTemp;
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = fTemp;
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from SHT25 sensor!");
      timeSinceLastRead = 0;
      return;
    }


    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    report(h, t, f);

    timeSinceLastRead = 0;
  }
  delay(100);
  timeSinceLastRead += 100;
}
