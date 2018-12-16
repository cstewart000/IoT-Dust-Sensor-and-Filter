
/*
     From the example OTA code from
     Will always allow upload of code via URL route




     //Youtube video ACROBOTIC
     https://www.youtube.com/watch?v=3aB85PuOQhY

     Python 2.7 (python 3.5 not supported) – https://www.python.org/
  Note: Windows users MUST select “Add python.exe to Path” while installing python 2.7 .


   Upload instructions
   ESP8266 must be connected to THE SAME network to upload codes.
   Make a request to the board to restart - this will make it receptive to new code OTA http://192.168.1.4/restart
   Select the network path as the COM port
   Upload the code from the IDE

*/


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

// TODO: Update to local network
const char* ssid = "HUAWEI-2XCNAA";
const char* password = "95749389";

//const char* ssid = "Hackland";
//const char* password = "hackland1";

//To enable OTA uploading, need web server
ESP8266WebServer server;
bool otaFlag = true;
static uint16_t TIME_BETWEEN_UPDATES = 15000;
uint16_t timeSinceLastUpdate = 0;

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Allows board to be restarted and to change the flag of ota_flag by accessing this route
  server.on("/restart", []() {
    server.send(200, "text/plain", "Restarting...");
    delay(1000); // Alow a second for the server to respond
    ESP.restart(); // Restart the application
  });

  // Change OTA flag to true
  server.on("/setflag", []() {
    server.send(200, "text/plain", "Setting the ota flag to 0 ...");
    otaFlag = true;
    timeSinceLastUpdate = 0;

    delay(1000); // Alow a second for the server to respond
  });

  // Change OTA flag to true
  server.on("/on", []() {
    server.send(200, "text/plain", "on!");
    digitalWrite(LED_BUILTIN, LOW);

  });

  // Change OTA flag to true
  server.on("/off", []() {
    server.send(200, "text/plain", "off!");
    digitalWrite(LED_BUILTIN, HIGH);
  });
  
  server.begin();
}

void loop() {
  if (otaFlag) {
    while (timeSinceLastUpdate < TIME_BETWEEN_UPDATES) {
      ArduinoOTA.handle();
      timeSinceLastUpdate = millis();
      delay(10); //required to allow handle to run
    }
    otaFlag = false;
  }

  server.handleClient();


}
