/*
 * GetMACAddress — utility sketch
 *
 * Flash this onto any ESP8266 / ESP32, open the serial monitor at
 * 115200 baud, and it prints the board's MAC address.
 * Copy the MAC into the config.h of the sketch that talks to this board.
 */

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>   // ESP32
#endif

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  delay(500);
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
