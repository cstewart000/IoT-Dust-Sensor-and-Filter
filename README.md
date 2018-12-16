# IoT Dust Sensor and Filter


// Filter

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
+ ESP8266 must be connected to THE SAME network to upload codes.
+ Make a request to the board to restart - this will make it receptive to new code OTA http://192.168.1.4/restart
+ Select the network path as the COM port
+ Upload the code from the IDE (you have a short time window after the restart has been called).

Circuit Description
+ Push button withpull down resister
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