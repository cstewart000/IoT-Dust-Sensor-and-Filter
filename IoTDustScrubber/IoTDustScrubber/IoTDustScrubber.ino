/*

  http://www.arduino.cc/en/Tutorial/Debounce
*/

#include <ESP8266WiFi.h>

// IoT settings
const char* ssid = "HUAWEI-2XCNAA";
const char* password = "95749389";

const char* host = "api.pushingbox.com";

// SLACK
const String slack_hook_url = "https://hooks.slack.com/services/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";
const String slack_icon_url = "https://visualpharm.com/assets/218/Small%20Robot-595b40b85ba036ed117dc489.svg";
const String slack_message = "test_message";
const String slack_username = "Air quality IoT device - Scrubber";

// :Hardware Settings
const int buttonPin = D2;    // the number of the pushbutton pin
const int scrubberSSRelay = D1;      // the number of the LED pin

// Variables will change:
int scrubberState = LOW;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers


void connectToWiFi();
void postToInternet();
void getFromInternet();
bool postMessageToSlack(String);


void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(scrubberSSRelay, OUTPUT);

  // set initial LED state
  digitalWrite(scrubberSSRelay, scrubberState);

  Serial.begin(9600);


  connectToWiFi();

  //attachInterrupt(digitalPinToInterrupt(buttonPin), IntCallback, RISING);

}

void loop() {

  // set the realay
  psuedoInterrupt();
  digitalWrite(scrubberSSRelay, scrubberState);

}

void IntCallback() {

  //Serial.println("Interrupt!" );

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    scrubberState = !scrubberState;
    lastDebounceTime = millis();

    Serial.print("Scrubber is: " );
    Serial.println(scrubberState, DEC);
    postToInternet();
  }

}

void psuedoInterrupt() {

  if (digitalRead(buttonPin)) {

    //Serial.println("Interrupt!" );

    if ((millis() - lastDebounceTime) > debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      scrubberState = !scrubberState;
      lastDebounceTime = millis();

      Serial.print("Scrubber is: " );
      Serial.println(scrubberState, DEC);
      postToInternet();
    }
  }

}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  postToInternet();
  //attachInterrupt(digitalPinToInterrupt(buttonPin), IntCallback, RISING);
}

void postToInternet() {
  String message;

  message += F("The scrubber is ");
  if(scrubberState){
     message += "on";
  }else{
     message += "off";
  }

  postMessageToSlack(message);


}


void getFromInternet() {
}


bool postMessageToSlack(String msg) {

  const char* host = "hooks.slack.com";
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpsPort = 443;
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed :-(");
    return false;
  }

  // We now create a URI for the request

  Serial.print("Posting to URL: ");
  Serial.println(slack_hook_url);

  String postData = "payload={\"link_names\": 1, \"icon_url\": \"" + slack_icon_url + "\", \"username\": \"" + slack_username + "\", \"text\": \"" + msg + "\"}";

  // This will send the request to the server
  client.print(String("POST ") + slack_hook_url + " HTTP/1.1\r\n" +
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



