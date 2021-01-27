#include "MQTT.h"

SYSTEM_THREAD(ENABLED);
void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * exp) iot.eclipse.org is Eclipse Open MQTT Broker: https://iot.eclipse.org/getting-started
 * MQTT client("mqtt.eclipse.org", 1883, callback, true);
 *      4th parameter : bool thread(default false.)
 *      SYSTEM_THREAD(ENABLED) settings : thread is true.
 **/
MQTT client("sample.com", 1883, callback, true);

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
    WiFi.on();
    WiFi.setCredentials("ssid", "pass");
    WiFi.connect();
    waitUntil(WiFi.ready);
    while (WiFi.localIP() == IPAddress()) {
    	delay(10);
    }

    // set LED control
    RGB.control(true);

    // connect to the server
    client.connect("sparkclient");

    // publish/subscribe
    if (client.isConnected()) {
        client.publish("outTopic/message","hello world");
        client.subscribe("inTopic/message");
    }
}

void loop() {
    if (client.isConnected())
      client.loop();
    delay(100);
}
