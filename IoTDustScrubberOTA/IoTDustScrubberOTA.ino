
/*
  A Program to Switch on an dust scrubber manually and from the internet. ]
  Can be adapted for any AC appliance with a push button start stop.

  Webserver instance run to enable http get requests to change state. Also suppurts over the air (OTA) firware updates.
  Webserver allows get request to switch on/off the relay. State of the circuit (SSR) is posted to a web agent hosted on 
  http://rocket.project-entropy.com

  Code written by: Cameron Stewart, cstewart000@gmail.com
  code hosted at: https://github.com/cstewart000/IoT-Dust-Sensor-and-Filter/blob/master/IoTDustScrubberOTA/IoTDustScrubberOTA.ino
  Blcg post of build at: https://hackingismakingisengineering.wordpress.com/2018/12/09/upgrading-an-air-filter-to-iot/

  Source material
  Example code "BasicOTA" and the youtube video from ACROBOTIC provided the basis for the OTA cfunction.
  https://www.youtube.com/watch?v=3aB85PuOQhY

  Python 2.7 (python 3.5 not supported) – https://www.python.org/
  Note: Windows users MUST select “Add python.exe to Path” while installing python 2.7 .

  OTA Upload instructions
  ESP8266 must be connected to THE SAME network to upload codes.
  Make a request to the board to restart - this will make it receptive to new code OTA http://192.168.1.4/restart
  Select the network path as the COM port
  Upload the code from the IDE (you have a short time window after the restart has been called).
  
  Circuit Description
  + Push button with  pull down resister
  + Solid State Relay
  + 5v Power supply unit - to provide power to NodeMCU
  + Fuse

  The 5v power supply is wired to the mains through the fuse.
  The live and neutral cores of the live Core is wired in parallel to the relay and spade terminals connect the live termional of 
  the motor to the realay. Spade terminals also connect the earth and neutral cores with allows the motor wiring loom to be 
  separated from the control circuitry.
  The case, motor, PSU and Mains are all earthed together on a binding post.  

  Wbhook for THIS PROJECT:
  http://rocket.project-entropy.com:5000/users/1/web_requests/8/airqualsecret

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

// TODO: Update to your h\web hook 
const char* WEB_HOOK = "http://rocket.project-entropy.com:5000/users/1/web_requests/8/airqualsecret";

//To enable OTA uploading, need web server
ESP8266WebServer server;
bool otaFlag = true;
static uint16_t TIME_BETWEEN_UPDATES = 15000;
uint16_t timeSinceLastUpdate = 0;

// Hardware Settings
const int BUTTON_PIN = D3;    // the number of the pushbutton pin
const int SCRUBBER_SSR_PIN = LED_BUILTIN;      // the number of the LED pin

// Variables will change:
int scrubberState = LOW;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long DEBOUNCE_DELAY = 2000;    // the debounce time; increase if the output flickers

void setup() {

  pinMode(BUTTON_PIN, INPUT);
  pinMode(SCRUBBER_SSR_PIN, OUTPUT);

  // set initial LED state
  digitalWrite(SCRUBBER_SSR_PIN, scrubberState);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }


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

    
  });

  // Change OTA flag to true
  server.on("/on", []() {
    server.send(200, "text/plain", "on!");
    scrubberState = false;
    digitalWrite(LED_BUILTIN, LOW);
  });

  // Change OTA flag to true
  server.on("/off", []() {
    server.send(200, "text/plain", "off!");
    scrubberState = true;
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

  // set the realay
  psuedoInterrupt();
  digitalWrite(SCRUBBER_SSR_PIN, scrubberState);
  postMessageToAPI(scrubberState);

}

void psuedoInterrupt() {

  if (digitalRead(BUTTON_PIN)) {

    //Serial.println("Interrupt!" );

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      // whatever the reading is at, it's been there for longer than the debounce
      scrubberState = !scrubberState;
      lastDebounceTime = millis();

      Serial.print("Scrubber is: " );
      Serial.println(scrubberState, DEC);
 
    }
  }
}

bool postMessageToAPI( bool scrubberState) {
  const char* host = "rocket.project-entropy.com";
  Serial.print("Connecting to ");
  Serial.println(WEB_HOOK);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpsPort = 5000;
  if (!client.connect(WEB_HOOK, httpsPort)) {
    Serial.println("Connection failed :-(");
    return false;
  }

  String postData = "data={\"scubber_state\": " + String(scrubberState) + "}";

  //String url = "/users/1/web_requests/8/airqualsecret";
 
  // This will send the request to the server
 client.print(String("POST ") + WEB_HOOK + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Connection: close" + "\r\n" +
               "Content-Length:" + postData.length() + "\r\n" +
               "\r\n" + postData);
  Serial.println("Request sent");
  
  String line = client.readStringUntil('\n');
  Serial.printf("Response code was: ");
  Serial.println(line);
  if (line.startsWith("HTTP/1.1 200 OK")) {
    return true;
  } else {
    return false;
  }
}
