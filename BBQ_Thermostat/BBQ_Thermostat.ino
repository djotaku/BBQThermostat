/* 
 *  BBQ Thermostat
 *  
 *  This design will read the temperature in a smoker and then adjust a fan connected to the air intake to control the temperature. 
 *  
 *  V1.0 - will measure the temperature and send the data via MQTT <- currently here
 *  V1.1 - will receive a target temp via MQTT
 *  V2.0 - will control the fan
 *  
 *  The current circuit:
 *  
 *  - Arduino MKR WiFi 1010
 *  - Arduino MKR THERM Shield attached
 *  - A K Type thermocouple temperature sensor connected to the shield
 *  
 *  Code licensed under GPL V3.0
 */

 //includes for WIFI
#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
//include for THERM shield
#include <Arduino_MKRTHERM.h>
//include for MQTT
#include <PubSubClient.h>

//secrets
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

//global variables
int status = WL_IDLE_STATUS;     // the Wifi radio's status
IPAddress server(192,168,1,14); //for Pubsubclient
WiFiClient wifiClient; //for Pubsubclient
PubSubClient client(wifiClient); //for Pubsubclient
int REPORTING_DELAY = 60000; // in milliseconds


void setup() {
  // Serial init code. This is for debugging and should be commented out when it's running at the smoker if debugging is not taking place
  Serial.begin(9600);
  //while (!Serial); //have to comment out or it won't work unless serial connection

  // make sure THERM shield will initialize
  if (!THERM.begin()) {
    Serial.println("Failed to initialize MKR THERM shield!");
    while (1);
  }

  //attempt to connect to Wifi Network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data: - this is also for debugging and should be commented out when running at the smoker if not debugging
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();

  //mqtt
  client.setServer(server, 1883);
  client.setCallback(callback);
}

void loop() {
  // debugging - wifi check
  printCurrentNet();
  // debugging - therm check
  Serial.print("Reference temperature ");
  Serial.print(THERM.readReferenceTemperature());
  Serial.println(" °C");
  Serial.print("Temperature ");
  Serial.print(THERM.readTemperature());
  Serial.println(" °C");
  Serial.println();
  //mqtt
  int smokertempnum = THERM.readTemperature()+3;
  char cstr[100];
  itoa(smokertempnum,cstr,10);
  if (!client.connected()) {
    reconnect();
  }
  if (smokertempnum > 500)
  {
    delay(REPORTING_DELAY); // this is to not report the error temp of 1073741824 which is pointless and will cause issues.
  }
  else
  {
    client.publish("smoker/temp",cstr);
    client.publish("smoker/WiFi/SSID",WiFi.SSID());
  }
  delay(REPORTING_DELAY);

}

///////////////////// WIFI Functions /////////////////////

// These are all for debugging and should be commented out when running at the smoker if not debugging
void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

//////////////////// END WIFI Functions ////////////////////

//////////// Pubsubclient Functions ///////////////

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
