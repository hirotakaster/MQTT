#include "MQTT.h"

char domain1[] = "iot.eclipse.org";
char domain2[] = "test.mosquitto.org";
byte server1[] = {192,168,1,2};
byte server2[] = {192,168,1,3};

void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * MQTT client("www.sample.com", 1883, callback);
 **/
MQTT client(server1, 1883, callback);

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
    Serial.begin();
    RGB.control(true);
    client.connect("sparkclient");
    if (client.isConnected()) {
        client.publish("192392391/message","hello world");
        client.subscribe("192392391/message");
    }
}

void loop() {
    if (client.isConnected()) {
        client.loop();
    }

    if (Serial.available() > 0) {
        long incomingChar = Serial.parseInt();
        switch (incomingChar) {
            case 1:
                client.setBroker(domain1, 1883);
                break;
            case 2:
                client.setBroker(domain2, 1883);
                break;
            case 3:
                client.setBroker(server1, 1883);
                break;
            case 4:
                client.setBroker(server2, 1883);
                break;
        }
        client.connect("sparkclient");
        if (client.isConnected()) {
            client.publish("192392391/message","hello world");
            client.subscribe("192392391/message");
        }
    }
}
