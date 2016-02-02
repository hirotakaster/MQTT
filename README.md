# MQTT for Photon, Spark Core.
<a href="http://mqtt.org/" target=_blank>MQTT</a> publish/subscribe library for Photon, Spark Core version 0.3.2.

## Source Code
This lightweight library source code are only 2 files. firmware -> MQTT.cpp, MQTT.h.

Application can use QOS0,1,2 and retain flag when send a publish message.

## Example
Some sample sketches included(firmware/examples/).
 - mqtttest.ino	: simple pub/sub sample. 
 - mqttqostest.ino : QoS1, QoS2 publish and callback sample.
