#include "MQTT/MQTT.h"

void callback(char* topic, byte* payload, unsigned int length);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * MQTT client("www.sample.com", 1883, callback);
 **/
MQTT client("server_name", 1883, callback);

// for QoS2 MQTTPUBREL message.
// this messageid maybe have store list or array structure.
uint16_t qos2messageid = 0;

// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    String message(p);

    if (message.equals("RED"))    
        RGB.color(255, 0, 0);
    else if (message.equals("GREEN"))    
        RGB.color(0, 255, 0);
    else if (message.equals("BLUE"))    
        RGB.color(0, 0, 255);
    else    
        RGB.color(255, 255, 255);
    delay(1000);
}

// QOS ack callback.
// MQTT server sendback message id ack, if use QOS1 or QOS2.
void qoscallback(unsigned int messageid) {
    Serial.print("Ack Message Id:");
    Serial.println(messageid);

    if (messageid == qos2messageid) {
        Serial.println("Release QoS2 Message");
        client.publishRelease(qos2messageid);
    }
}

void setup() {
    Serial.begin(9600);
    RGB.control(true);
    
    // connect to the server
    client.connect("sparkclient");

    // add qos callback
    client.addQosCallback(qoscallback);

    // publish/subscribe
    if (client.isConnected()) {
        // get messageid parameter at 4.

        uint16_t messageid;
        client.publish("/outTopic", "hello world QOS1", MQTT::QOS1, &messageid);
        Serial.println(messageid);

        client.publish("/outTopic", "hello world QOS2", MQTT::QOS2, &messageid);
        Serial.println(messageid);

        // save QoS2 message id.
        qos2messageid = messageid;

        client.subscribe("/inTopic");
    }
}

void loop() {
    if (client.isConnected())
        client.loop();
}
