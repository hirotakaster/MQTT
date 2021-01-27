# MQTT for Photon, Spark Core
<a href="http://mqtt.org/" target=_blank>MQTT</a> publish/subscribe library for Photon, Argon, Tracker One...etc version 0.4.30.

## Source Code
This lightweight library source code is only 2 files. firmware -> MQTT.cpp, MQTT.h.

The application can use QoS 0, 1, 2 and the retain flag when publishing a message.

## Example
Some sample sketches for Spark Core and Photon included (firmware/examples/).
 - mqtttest.ino	: simple pub/sub sample. 
 - mqttqostest.ino : QoS1, QoS2 publish and callback sample.
 - mqttSwitchBroker.ino : Example of how to switch to different brokers.
 - threadtest.ino : Example of SYSTEM_THREAD(ENABLED).

## developer examples
some applications use MQTT with Photon. here are developer's reference examples.
- <a href="http://www.instructables.com/id/Spark-Core-Photon-and-CloudMQTT/" target="_blank">Spark Core / Photon and CloudMQTT</a>
- <a href="https://www.hackster.io/anasdalintakam/mqtt-publish-subscribe-using-rpi-esp-and-photon-864fe9#toc--particle-photon-as-mqtt-client-2" target="_blank">MQTT Publish-Subscribe Using Rpi, ESP and Photon</a>
- <a href="http://www.kevinhoyt.com/2016/04/27/particle-photon-on-watson-iot/" target="_blank">Particle Photon on Watson IoT</a>
- <a href="https://developer.ibm.com/recipes/tutorials/connecting-a-iot-device-of-the-watson-conversation-cardashboard-app/" target="_blank">Connecting IoT devices to the Watson Conversation Car-Dashboard app</a>
- <a href="https://jp.mathworks.com/help/thingspeak/mqtt-api.html" target="_blank">ThingSpeak MQTT API</a>
- <a href="https://www.losant.com/blog/how-to-connect-a-particle-photon-to-the-losant-iot-platform" target="_blank">How to Connect a Particle Photon to the Losant IoT Platform</a>
- <a href="https://medium.com/@stevecaldwell/how-i-hacked-my-humidor-with-losant-and-a-particle-photon-84342744755b#.b68apdmo1" target="_blank">How I Hacked my Humidor with Losant and a Particle Photon</a>
- <a href="https://www.digikey.jp/ja/maker/projects/how-to-build-a-photon-mqtt-logger/876ce49a8f914f0799a0f8b94519acc1" target="_blank">How to Build a Photon MQTT Logger</a>
- <a href="https://ubidots.com/docs/devices/particleMQTT.html" target="_blank">Particle and Ubidots using MQTT</a>
- <a href="https://www.twilio.com/docs/quickstart/sync-iot/mqtt-particle-photon-sync-iot" target="_blank">Using Twilio Sync with MQTT on a Particle Photon</a>

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
- Test your MQTT server and port (default 1883) with the mosquitto_pub/sub command. 
- Check your network environments. Make sure your MQTT server can reach the Internet through your firewall. 
- Verify your subscribe/publish topic name is correct.
- Perhaps device firmware network stack is failed. check your firmware version and bugs.
- If you are using MQTT-TLS, make sure your RooT CA pem file, client key, certifications is valid.
- Several MQTT servers will disconnect the first connection when you use the same user_id. When the application calls the connect method, use a different user_id on every device as connect method's second argument. Using the MAC address as a user_id is suggested.
<pre>
   // device.1
   client.connect("spark-client", "user_1", "password1");
   // other devices...
   client.connect("spark-client", "user_others", "password1");
</pre>

### I want to change MQTT keepalive timeout.
MQTT keepalive timeout is defined "MQTT_DEFAULT_KEEPALIVE 15"(15 sec) in header file. You can change the keepalive timeout in constructor.
<pre>
    MQTT client("server_name", 1883, callback); // default: send keepalive packet to MQTT server every 15sec.
    MQTT client("server_name", 1883, 30, callback); // keepalive timeout is 30 sec.
</pre>

### Want to use over the 255 byte message size.
In this library, the maximum MQTT message size is defined as "MQTT_MAX_PACKET_SIZE 255" in the header file. If you want to use over 255 bytes, use the constructor's last argument.
<pre>
    MQTT client("server_name", 1883, callback); // default 255 bytes
    MQTT client("server_name", 1883, MQTT_DEFAULT_KEEPALIVE, callback, 512); // max 512 bytes
</pre>

### Can I use on old firmware?
No, use the default latest firmware. I tested this library on the default latest firmware or the latest pre-release version. If you use an old firmware it may not work well and is not supported.

### Bug or Problem?
First, check the <a href="https://community.particle.io/" target="_blank">Particle community site<a/>. But if your problem is not resolved, please create an issue with the problem details.

### Pull Request 
If you have a bug fix or feature, please send a pull request.
Thanks for all developers' pull requests!

