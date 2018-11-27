

/*********************************************************************************************************

  File                : DustSensor
  Hardware Environment:
  Build Environment   : Arduino
  Version             : V1.0.5-r2
  By                  : WaveShare

                                   (c) Copyright 2005-2011, WaveShare
                                        http://www.waveshare.net
                                        http://www.waveshare.com
                                           All Rights Reserved

  Google sheet element:
  <input type="text" class="quantumWizTextinputPaperinputInput exportInput" jsname="YPqjbf" autocomplete="off" tabindex="0" aria-label="Whats the dust level?" aria-describedby="i.desc.421979024 i.err.421979024" name="entry.1639810656" value="" dir="auto" data-initial-dir="auto" data-initial-value="" aria-invalid="false">
entry.1639810656
data-initial-value

DHT11 tutorial
http://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-an-arduino/
*********************************************************************************************************/
#include <ESP8266WiFi.h>
#include <DHTesp.h>


// Dust monitor settings
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000

//MQ-2 Sensor threshold
int sensorThres = 400;

// IoT settings
const char* ssid = "HUAWEI-2XCNAA";
const char* password = "95749389";

const char* host = "api.pushingbox.com";

// SLACK
const String slack_hook_url = "https://hooks.slack.com/services/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";
const String slack_icon_url = "https://visualpharm.com/assets/218/Small%20Robot-595b40b85ba036ed117dc489.svg";
const String slack_message = "test_message";
const String slack_username = "Air quality IoT device";


// I/O define
const int dht11 = D1; //temp and humidity
const int mq2 = D2; //Gas sensor
const int hcsr501 = D3; //PIR sensor
const int iled = D7; //drive the led of sensor
const int vout = A0; //analog input


// variable
float density, voltage, temperature, humidity;
int   adcvalue;
static char outstr[15];
bool movement = false;

String message;

DHTesp dht;

int filter(int m);

void setup(void)
{

  // initialise the DHT
  dht.setup(dht11, DHTesp::DHT11);

  
  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW); //iled default closed
  Serial.begin(9600);  //send and receive at 9600 baud
  Serial.print("Dust Sensor");

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
}

void loop(void)
{
  // DHT 
  delay(dht.getMinimumSamplingPeriod());

  // Read temp and humidity
  //int chk = DHT.read11(dht11);
  temperature = dht.getTemperature();
  humidity = dht.getHumidity();

  // check movement
  movement = digitalRead(hcsr501);

  
  // read particle sensor
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);

  adcvalue = filter(adcvalue);
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;// convert voltage (mv)

  // voltage to density
  if (voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else
    density = 0;

  dtostrf(density , 7, 3, outstr);

  // Print temp and humidity
  Serial.print("The current temperature is: ");
  Serial.print(temperature, 1);
  Serial.print(" 'C, humidity: ");
  Serial.println(humidity, 1);

  // Print Gas
  Serial.print("The current temperature is: ");
  Serial.print(temperature);
  Serial.print(" 'C, humidity: ");
  Serial.println(humidity);
  
  // Print movement
  Serial.print("Movement detected: ");
  Serial.println(movement);


  // print result to console - post to internet
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");

 /*
  char outstrMovement[10];
  dtostrf(movement , 7, 3, outstrMovement);

  char outstrDensity[10];
  dtostrf(density , 7, 3, outstrDensity);
  
  //message += "Movement detected: " + outstrMovement + "\n";
  message += "The current dust concentration is: "+  density + " ug/m3\n";
  */

  message += F("Movement detected: ");
  message += String(movement, 2);
  message += F("\n");

  message += F("The current dust concentration is: ");
  message += String(density, 6);
  message += F(" ug/m3\n");

  message += F("The temperature is: ");
  message += String(temperature, 1);
  message += F(" 'C\n");
  
  message += F("The humidity is: ");
  message += String(humidity, 1);
  message += F(" %\n");
    
  postMessageToSlack(message);
  
  delay(3600000);
}

/*
  private function
*/
int filter(int m) {
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;

  if (flag_first == 0)
  {
    flag_first = 1;

    for (i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for (i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];

    i = sum / 10.0;
    return i;
  }
}


bool postMessageToSlack(String msg)
{
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

  String postData="payload={\"link_names\": 1, \"icon_url\": \"" + slack_icon_url + "\", \"username\": \"" + slack_username + "\", \"text\": \"" + msg + "\"}";

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



















