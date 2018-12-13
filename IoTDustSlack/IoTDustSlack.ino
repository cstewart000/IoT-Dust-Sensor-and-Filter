
#include <ESP8266WiFi.h>
#include <DHTesp.h>


// Dust monitor settings
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000


#define POST_TIME_MS 5000


//MQ-2 Sensor threshold
int sensorThres = 400;

// IoT settings
const char* ssid = "Hackland";
const char* password = "hackland1";

const char* host = "api.pushingbox.com";

// SLACK
const String slack_hook_url = "https://hooks.slack.com/services/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";
const String slack_icon_url = "https://visualpharm.com/assets/218/Small%20Robot-595b40b85ba036ed117dc489.svg";
const String slack_message = "test_message";
const String slack_username = "Air quality IoT device";


// I/O define
//Power pins
const int dustSensorPowerPin = D7; //drive the led of dust sensor
//const int pirSensorPowerPin = D1; //PIR sensor Power
//const int gasSensorPowerPin = D4; //Gas sensor Power
//const int microphnePowerPin = D4; //Microphone sensor Power

//read pins
const int PIRSensorDDataPin = D3; //PIR sensor
const int tempHumiditySensorDataPin = D1; //temp and humidity

// Common Analog in NB: 3 of the sensors are analog and need to be read one at a time with diodes to isolate the circuits from each other
const int commonAnalogInputPin = A0; //analog input


// status variable
float density, voltage, temperature, humidity;
int adcvalue, noiselevel, gaslevel;
bool movementPIRSensor = false;

static char outstr[15];
bool connected;

// DHT object variable
DHTesp dht;

/*
int filter(int m);
void readAnalogSensor(int digitalPinToPower);
void readMicrophone(int analogValue);
void readDustSensor(int analogValue);
void readMQ2GasSensor(int analogValue);
void readPIRSensor();
void readTempAndHumiditySensor();
int readAnalogSensor(int digitalPinToPower);
*/

//Posting functions
void printSensorValuesToConsole();
bool postMessageToSlack(String msg);
void connectToInternet();

void setup(void){

  // Initialise Serial
  Serial.begin(9600);  //send and receive at 9600 baud
  
  connectToInternet();

  // initialise the DHT
  dht.setup(tempHumiditySensorDataPin, DHTesp::DHT11);

  // Initialise outputs and modes
  pinMode(dustSensorPowerPin, OUTPUT);
  digitalWrite(dustSensorPowerPin, LOW);

  //pinMode(pirSensorPowerPin, OUTPUT);
  //digitalWrite(pirSensorPowerPin, LOW);

  //pinMode(gasSensorPowerPin, OUTPUT);
  //digitalWrite(gasSensorPowerPin, LOW);

}

void loop(void){
  connected = (WiFi.status() == WL_CONNECTED);
   
  // DHT 
  //delay(dht.getMinimumSamplingPeriod());
  Serial.printf("dht sampling period: %d\n" ,dht.getMinimumSamplingPeriod());

  readTempAndHumiditySensor();
  
  // check movementPIRSensor
  readPIRSensor();

  //Microphone
  //readMicrophone(readAnalogSensor(microphnePowerPin));
  
  // read particle sensor
  readDustSensor(readAnalogSensor(dustSensorPowerPin));

   // read particle sensor
  //readMQ2GasSensor(readAnalogSensor(gasSensorPowerPin)); 

  // Debug sensor values to console
  printSensorValuesToConsole();

  //Serial.println("broadcast");
  String message = composePostMessage();
  //Serial.println("message");
  if(connected)
  {
    postMessageToSlack(message);
  } 
  
  delay(POST_TIME_MS);
}

//filter for dust sensor values.
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


void connectToInternet(){

  Serial.println("connectToInternet");

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

  connected = true;
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
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
    Serial.println("Connection :-(");
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

int readAnalogSensor(int digitalPinToPower){

  digitalWrite(digitalPinToPower,HIGH);
  delay(300); //TODO check delay on function

  int analogValue = analogRead(commonAnalogInputPin);

  digitalWrite(digitalPinToPower,LOW);

  //Degu
  Serial.printf("analog vlaue of: %d read on pin %d", analogValue, digitalPinToPower);

  return analogValue;
  
}

void readMicrophone(int analogValue){

  noiselevel = analogValue;
  
}

void readMQ2GasSensor(int analogValue){

  gaslevel = analogValue;
}

void readDustSensor(int analogValue){


  adcvalue = filter(analogValue);
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;// convert voltage (mv)

  // voltage to density
  if (voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else
    density = 0;


}

void readPIRSensor(){

  movementPIRSensor = digitalRead(PIRSensorDDataPin);

}
void readTempAndHumiditySensor(){
    // Read temp and humidity
  //int chk = DHT.read11(tempHumiditySensorDataPin);
  temperature = dht.getTemperature();
  humidity = dht.getHumidity();
}


// Print temp and humidity
void printSensorValuesToConsole(){
  Serial.println();
  
  Serial.print("The current temperature is: ");
  Serial.print(temperature, 1);
  Serial.print(" 'C, humidity: ");
  Serial.println(humidity, 1);

  // Print Gas
  //see https://components101.com/mq2-gas-sensor under procedure to measure ppm, you need to calibrate and calculate constants
  Serial.print("The current gas level is: ");
  Serial.print(gaslevel);
  Serial.print(" ppm: ");
  
  // Print movementPIRSensor
  Serial.print("movementPIRSensor detected: ");
  Serial.println(movementPIRSensor);


  // print result to console - post to internet
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.println(" ug/m3\n");
}

String composePostMessage(){
//Serial.println("composePostMessage");
  String message = "";
  
  message += F("movementPIRSensor detected: ");
  message += String(movementPIRSensor, 2);
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

  return message;
}


