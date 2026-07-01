#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Set WiFi to Station mode to get the correct MAC address
  WiFi.mode(WIFI_STA);
  
  Serial.println("\n----------------------------------");
  Serial.println("NodeMCU ESP8266 MAC Address Finder");
  Serial.println("----------------------------------");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("----------------------------------");
  Serial.println("Copy the MAC Address above and paste it into TX.ino!");
}

void loop() {
  // Do nothing
}
