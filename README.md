# MQTT for Photon, Spark Core
<a href="http://mqtt.org/" target=_blank>MQTT</a> publish/subscribe library for Photon, Spark Core version 0.4.26.

## Source Code
This lightweight library source code are only 2 files. firmware -> MQTT.cpp, MQTT.h.

Application can use QOS0,1,2 and retain flag when send a publish message.

## Example
Some sample sketches for Spark Core and Photon included(firmware/examples/).
 - mqtttest.ino	: simple pub/sub sample. 
 - mqttqostest.ino : QoS1, QoS2 publish and callback sample.

## developer examples
some applications use MQTT with Photon. here are developer's reference examples.
- <a href="http://www.instructables.com/id/Spark-Core-Photon-and-CloudMQTT/" target="_blank">Spark Core / Photon and CloudMQTT</a>
- <a href="https://www.hackster.io/anasdalintakam/mqtt-publish-subscribe-using-rpi-esp-and-photon-864fe9#toc--particle-photon-as-mqtt-client-2" target="_blank">MQTT Publish-Subscribe Using Rpi, ESP and Photon</a>
- <a href="http://www.kevinhoyt.com/2016/04/27/particle-photon-on-watson-iot/" target="_blank">Particle Photon on Watson IoT</a>
- <a href="https://developer.ibm.com/recipes/tutorials/connecting-a-iot-device-of-the-watson-conversation-cardashboard-app/" target="_blank">Connecting IoT devices to the Watson Conversation Car-Dashboard app</a>
- <a href="https://jp.mathworks.com/help/thingspeak/mqtt-api.html" target="_blank">ThingSpeak MQTT API</a>
- <a href="https://www.losant.com/blog/how-to-connect-a-particle-photon-to-the-losant-iot-platform" target="_blank">HOW TO CONNECT A PARTICLE PHOTON TO THE LOSANT IOT PLATFORM</a>
- <a href="https://medium.com/@stevecaldwell/how-i-hacked-my-humidor-with-losant-and-a-particle-photon-84342744755b#.b68apdmo1" target="_blank">How I Hacked my Humidor with Losant and a Particle Photon</a>
- <a href="https://developer.artik.io/documentation/advanced-concepts/mqtt/color-mqtt.html" target="_blank">ARTIK as MQTT Message Broker</a>
- <a href="https://ubidots.com/docs/devices/particleMQTT.html" target="_blank">Particle and Ubidots using MQTT</a>
- <a href="https://www.twilio.com/docs/quickstart/sync-iot/mqtt-particle-photon-sync-iot" target="_blank">USING TWILIO SYNC WITH MQTT ON A PARTICLE PHOTON</a>

## sample source
```
#include "application.h"
#include "MQTT.h"

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("iot.eclipse.org", 1883, callback);

// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    if (!strcmp(p, "RED"))
        RGB.color(255, 0, 0);
    else if (!strcmp(p, "GREEN"))
        RGB.color(0, 255, 0);
    else if (!strcmp(p, "BLUE"))
        RGB.color(0, 0, 255);
    else
        RGB.color(255, 255, 255);
    delay(1000);
}


void setup() {
    RGB.control(true);

    // connect to the server(unique id by Time.now())
    client.connect("sparkclient_" + String(Time.now()));

    // publish/subscribe
    if (client.isConnected()) {
        client.publish("outTopic/message","hello world");
        client.subscribe("inTopic/message");
    }
}

void loop() {
    if (client.isConnected())
        client.loop();
}
```
## FAQ
### Can't connect/publish/subscribe to the MQTT server?
- Check your MQTT server and port(default 1883) is really working with the mosquitto_pub/sub command. And maybe your MQTT server can't connect from Internet because of firewall. Check your network environments.
- Check your subscribe/publish topic name is really matched.
- Perhaps device firmware network stack is failed. check your firmware version and bugs.
- If you use MQTT-TLS, check your RooT CA pem file, client key, certifications is okay or not.
- Several MQTT server will disconnect to the 1st connection when you use the same user_id. When the application call the connect method, use different user_id in every devices in connect method's 2nd argument. Use MAC address as a user_id will be better.
<pre>
   // device.1
   client.connect("spark-client", "user_1", "password1");
   // other devices...
   client.connect("spark-client", "user_others", "password1");
</pre>

### I want to change MQTT keep alive timeout.
MQTT keepalive timeout is defined "MQTT_DEFAULT_KEEPALIVE 15"(15 sec) in header file. You can change the keepalive timeout in constructor.
<pre>
    MQTT client("server_name", 1883, callback); // default: send keepalive packet to MQTT server in every 15sec.
    MQTT client("server_name", 1883, 30, callback); // keepliave timeout is 30sec.
</pre>

### Want to use over the 255 message size.
In this library, max MQTT message size is defined "MQTT_MAX_PACKET_SIZE 255" in header file. But If you want to use over 255bytes, use the constructor 4th argument.
<pre>
    MQTT client("server_name", 1883, callback); // default 255bytes
    MQTT client("server_name", 1883, MQTT_DEFAULT_KEEPALIVE, callback, 512); // max 512bytes
</pre>

### Can I use on old firmware?
No, use default latest firmware. I test this library on default latest firmware or latest pre-release version. If you really want to use old firmware(I think don't need that case), maybe it can't work well and it is out of my assumption.

### Bug or Problem?
First of all, check the <a href="https://community.particle.io/" target="_blank">Particle community site<a/>. But still problem will not solve, please send a bug-fixed diff and Pull request or problem details to issue.
