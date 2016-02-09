#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MQTT.h>

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

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


// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);

  Serial.println(message);
  delay(1000);
}


void setup() {
  Serial.begin(9600);

  // Get IP address from DHCP Server
  Ethernet.begin(mac);
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
    
  // connect to the server
  client.connect("sparkclient");

  // add qos callback
  client.addQosCallback(qoscallback);

  // publish/subscribe
  if (client.isConnected()) {
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

