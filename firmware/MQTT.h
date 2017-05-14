/*
MQTT library for Particle Core, Photon, Arduino
This software is released under the MIT License.
Copyright (c) 2014 Hirotaka Niisato
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
Much of the code was inspired by Arduino Nicholas pubsubclient
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (c) 2008-2012 Nicholas O'Leary
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------
*/

#ifndef __MQTT_H_
#define __MQTT_H_

#include "application.h"
#include "spark_wiring_string.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_usbserial.h"

// MQTT_MAX_PACKET_SIZE : Maximum packet size
// this size is total of [MQTT Header(Max:5byte) + Topic Name Length + Topic Name + Message ID(QoS1|2) + Payload]
#define MQTT_MAX_PACKET_SIZE 255

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_DEFAULT_KEEPALIVE 15

#define MQTTPROTOCOLVERSION 3
#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved


// for debugging.
#define DEBUG_MQTT_SERIAL_OUTPUT       1
#if defined(DEBUG_MQTT_SERIAL_OUTPUT)
#define debug_print(fmt, ...)  Serial.printf("[DEBUG] MQTT " fmt, ##__VA_ARGS__)
#else /* !DEBUG_MQTT_SERIAL_OUTPUT */
  #define debug_print(fmt, ...) ((void)0)
#endif /* DEBUG_MQTT_SERIAL_OUTPUT */


class MQTT {
/** types */
public:
typedef enum {
    QOS0 = 0,
    QOS1 = 1,
    QOS2 = 2,
} EMQTT_QOS;

typedef enum {
    CONN_ACCEPT = 0,
    CONN_UNACCEPTABLE_PROCOTOL = 1,
    CONN_ID_REJECT = 2,
    CONN_SERVER_UNAVAILALE = 3,
    CONN_BAD_USER_PASSWORD = 4,
    CONN_NOT_AUTHORIZED = 5
} EMQTT_CONNACK_RESPONSE;

private:
    TCPClient *_client;
    uint8_t *buffer;
    uint16_t nextMsgId;
    unsigned long lastOutActivity;
    unsigned long lastInActivity;
    bool pingOutstanding;
    void (*callback)(char*,uint8_t*,unsigned int);
    void (*qoscallback)(unsigned int);
    uint16_t readPacket(uint8_t*);
    uint8_t readByte();
    bool write(uint8_t header, uint8_t* buf, uint16_t length);
    uint16_t writeString(const char* string, uint8_t* buf, uint16_t pos);
    String domain;
    uint8_t *ip;
    uint16_t port;
    int keepalive;
    uint16_t maxpacketsize;

    void initialize(char* domain, uint8_t *ip, uint16_t port, int keepalive, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);

public:
    MQTT();

    MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int));

    MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);

    MQTT(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int));

    MQTT(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);

    MQTT(char* domain, uint16_t port, int keepalive, void (*callback)(char*,uint8_t*,unsigned int));

    MQTT(char* domain, uint16_t port, int keepalive, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);

    MQTT(uint8_t *ip, uint16_t port, int keepalive, void (*callback)(char*,uint8_t*,unsigned int));

    MQTT(uint8_t *ip, uint16_t port, int keepalive, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);

    ~MQTT();

    void setBroker(char* domain, uint16_t port);
    void setBroker(uint8_t *ip, uint16_t port);

    bool connect(const char *id);
    bool connect(const char *id, const char *user, const char *pass);
    bool connect(const char *id, const char *user, EMQTT_QOS, uint8_t, const char *pass);
    bool connect(const char *id, const char *user, const char *pass, const char* willTopic, EMQTT_QOS willQos, uint8_t willRetain, const char* willMessage);
    bool connect(const char *id, const char *user, const char *pass, const char* willTopic, EMQTT_QOS willQos, uint8_t willRetain, const char* willMessage, uint8_t cleanSession);
    void disconnect();

    bool publish(const char *topic, const char* payload);
    bool publish(const char *topic, const char* payload, EMQTT_QOS qos, uint16_t *messageid = NULL);
    bool publish(const char *topic, const char* payload, EMQTT_QOS qos, bool dup, uint16_t *messageid = NULL);
    bool publish(const char *topic, const uint8_t *pyaload, unsigned int plength);
    bool publish(const char *topic, const uint8_t *payload, unsigned int plength, EMQTT_QOS qos, uint16_t *messageid = NULL);
    bool publish(const char *topic, const uint8_t *payload, unsigned int plength, EMQTT_QOS qos, bool dup, uint16_t *messageid = NULL);
    bool publish(const char *topic, const uint8_t *payload, unsigned int plength, bool retain);
    bool publish(const char *topic, const uint8_t *payload, unsigned int plength, bool retain, EMQTT_QOS qos, uint16_t *messageid = NULL);
    bool publish(const char *topic, const uint8_t *payload, unsigned int plength, bool retain, EMQTT_QOS qos, bool dup, uint16_t *messageid);
    void addQosCallback(void (*qoscallback)(unsigned int));
    bool publishRelease(uint16_t messageid);

    bool subscribe(const char *topic);
    bool subscribe(const char *topic, EMQTT_QOS);
    bool unsubscribe(const char *topic);
    bool loop();
    bool isConnected();
};

#endif  // __MQTT_H_

