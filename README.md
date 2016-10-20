# MQTT library for Arduino compatible with Spark Photon, Spark Core.
<a href="http://mqtt.org/" target=_blank>MQTT</a> publish/subscribe library for Arduino version 0.4.2.

## Source Code
This lightweight library source code are only 2 files. MQTT.cpp, MQTT.h.
Application can use QOS0,1,2 and retain flag when send a publish message.

## How to use
Download this source code branch zip file and extract to the Arduino libraries directory or checkout repository. Here is checkout on MacOS X.

    cd $HOME/Documents/Arduino/libraries/
    git clone -b arduino https://github.com/hirotakaster/MQTT MQTTClient
    # restart Arduino IDE, you can find MQTTCilent examples.

## Example
This library use Arduino Ethernet Client, so application can use this library on generic Arduino hardware(YUN, Ethernet Shield, ESP8266...others). Some sample sketches included(examples/).
 - SimplePubSub	: simple pub/sub sample. 
 - QoSPubSub : QoS1, QoS2 publish and callback sample.

## Particle Photon, Core compatible
This library is Particle Photon, Core compatible. That's version is <a href="https://github.com/hirotakaster/MQTT">here</a>.
