#include "MQTT.h"
#include "application.h"

#define LOGGING

#define MQTTQOS0_HEADER_MASK        (0 << 1)
#define MQTTQOS1_HEADER_MASK        (1 << 1)
#define MQTTQOS2_HEADER_MASK        (2 << 1)

MQTT::MQTT() {
    this->ip = NULL;
}

MQTT::MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int)) {
    this->callback = callback;
    this->qoscallback = NULL;
    this->domain = domain;
    this->port = port;
    this->ip = NULL;
}

MQTT::MQTT(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int)) {
    this->callback = callback;
    this->qoscallback = NULL;
    this->ip = ip;
    this->port = port;
}

void MQTT::addQosCallback(void (*qoscallback)(unsigned int)) {
    this->qoscallback = qoscallback;
}

bool MQTT::connect(const char *id) {
    return connect(id,NULL,NULL,0,QOS0,0,0);
}

bool MQTT::connect(const char *id, const char *user, const char *pass) {
    return connect(id,user,pass,0,QOS0,0,0);
}

bool MQTT::connect(const char *id, const char* willTopic, EMQTT_QOS willQos, uint8_t willRetain, const char* willMessage) {
    return connect(id,NULL,NULL,willTopic,willQos,willRetain,willMessage);
}

bool MQTT::connect(const char *id, const char *user, const char *pass, const char* willTopic, EMQTT_QOS willQos, uint8_t willRetain, const char* willMessage) {
    if (!isConnected()) {
        int result = 0;
        if (ip == NULL)
            result = _client.connect(this->domain.c_str(), this->port);
        else
            result = _client.connect(this->ip, this->port);

        if (result) {
            nextMsgId = 1;
            uint8_t d[9] = {0x00,0x06,'M','Q','I','s','d','p',MQTTPROTOCOLVERSION};
            // Leave room in the buffer for header and variable length field
            uint16_t length = 5;
            unsigned int j;
            for (j = 0;j<9;j++) {
                buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic) {
                v = 0x06|(willQos<<3)|(willRetain<<5);
            } else {
                v = 0x02;
            }

            if(user != NULL) {
                v = v|0x80;

                if(pass != NULL) {
                    v = v|(0x80>>1);
                }
            }

            buffer[length++] = v;

            buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
            buffer[length++] = ((MQTT_KEEPALIVE) & 0xFF);
            length = writeString(id, buffer, length);
            if (willTopic) {
                length = writeString(willTopic, buffer, length);
                length = writeString(willMessage, buffer, length);
            }

            if(user != NULL) {
                length = writeString(user,buffer,length);
                if(pass != NULL) {
                    length = writeString(pass,buffer,length);
                }
            }
         
            write(MQTTCONNECT, buffer, length-5);
            lastInActivity = lastOutActivity = millis();
         
            while (!_client.available()) {
                unsigned long t = millis();
                if (t-lastInActivity > MQTT_KEEPALIVE*1000UL) {
                    _client.stop();
                    return false;
                }
            }
            uint8_t llen;
            uint16_t len = readPacket(&llen);
         
            if (len == 4 && buffer[3] == 0) {
                lastInActivity = millis();
                pingOutstanding = false;
                return true;
            }
        }
        _client.stop();
    }
    return false;
}

uint8_t MQTT::readByte() {
    while(!_client.available()) {}
    return _client.read();
}

uint16_t MQTT::readPacket(uint8_t* lengthLength) {
    uint16_t len = 0;
    buffer[len++] = readByte();
    bool isPublish = (buffer[0]&0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint16_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint8_t start = 0;
   
    do {
        digit = readByte();
        buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);
    *lengthLength = len-1;

    if (isPublish) {
        // Read in topic length to calculate bytes to skip over for Stream writing
        buffer[len++] = readByte();
        buffer[len++] = readByte();
        skip = (buffer[*lengthLength+1]<<8)+buffer[*lengthLength+2];
        start = 2;
        if (buffer[0] & MQTTQOS1_HEADER_MASK) {
            // skip message id
            skip += 2;
        }
    }

    for (uint16_t i = start;i<length;i++) {
        digit = readByte();
        if (len < MQTT_MAX_PACKET_SIZE) {
            buffer[len] = digit;
        }
        len++;
    }
   
    if (len > MQTT_MAX_PACKET_SIZE) {
        len = 0; // This will cause the packet to be ignored.
    }

    return len;
}

bool MQTT::loop() {
    if (isConnected()) {
        unsigned long t = millis();
        if ((t - lastInActivity > MQTT_KEEPALIVE*1000UL) || (t - lastOutActivity > MQTT_KEEPALIVE*1000UL)) {
            if (pingOutstanding) {
                _client.stop();
                return false;
            } else {
                buffer[0] = MQTTPINGREQ;
                buffer[1] = 0;
                _client.write(buffer,2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (_client.available()) {
            uint8_t llen;
            uint16_t len = readPacket(&llen);
            uint16_t msgId = 0;
            uint8_t *payload;
            if (len > 0) {
                lastInActivity = t;
                uint8_t type = buffer[0]&0xF0;
                if (type == MQTTPUBLISH) {
                    if (callback) {
                        uint16_t tl = (buffer[llen+1]<<8)+buffer[llen+2];
                        char topic[tl+1];
                        for (uint16_t i=0;i<tl;i++) {
                            topic[i] = buffer[llen+3+i];
                        }
                        topic[tl] = 0;
                        // msgId only present for QOS>0
                        if ((buffer[0]&0x06) == MQTTQOS1_HEADER_MASK) {
                            msgId = (buffer[llen+3+tl]<<8)+buffer[llen+3+tl+1];
                            payload = buffer+llen+3+tl+2;
                            callback(topic,payload,len-llen-3-tl-2);
                    
                            buffer[0] = MQTTPUBACK;
                            buffer[1] = 2;
                            buffer[2] = (msgId >> 8);
                            buffer[3] = (msgId & 0xFF);
                            _client.write(buffer,4);
                            lastOutActivity = t;
                        } else {
                            payload = buffer+llen+3+tl;
                            callback(topic,payload,len-llen-3-tl);
                        }
                    }
                } else if (type == MQTTPUBACK || type == MQTTPUBREC) {
                    if (qoscallback) {
                        // msgId only present for QOS==0
                        if (len == 4 && (buffer[0]&0x06) == MQTTQOS0_HEADER_MASK) {
                            msgId = (buffer[2]<<8)+buffer[3];
                            this->qoscallback(msgId);
                        }
                    }
                } else if (type == MQTTPUBCOMP) {
                    // TODO:if something...
                } else if (type == MQTTSUBACK) {
                    // if something...
                } else if (type == MQTTPINGREQ) {
                    buffer[0] = MQTTPINGRESP;
                    buffer[1] = 0;
                    _client.write(buffer,2);
                } else if (type == MQTTPINGRESP) {
                    pingOutstanding = false;
                }
            }
        }
        return true;
    }
    return false;
}

bool MQTT::publish(const char* topic, const char* payload) {
    return publish(topic, (uint8_t*)payload, strlen(payload), false, QOS0, NULL);
}

bool MQTT::publish(const char * topic, const char* payload, EMQTT_QOS qos, uint16_t *messageid) {
    return publish(topic, (uint8_t*)payload, strlen(payload), false, qos, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength) {
    return publish(topic, payload, plength, false, QOS0, NULL);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, EMQTT_QOS qos, uint16_t *messageid) {
    return publish(topic, payload, plength, false, qos, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool kept) {
    return publish(topic, payload, plength, kept, QOS0, NULL);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool kept, EMQTT_QOS qos, uint16_t *messageid) {
    if (isConnected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = 5;
        memset(buffer, 0, sizeof(buffer));

        length = writeString(topic, buffer, length);

        if (qos == QOS2 || qos == QOS1) {
            *messageid = nextMsgId++;
            buffer[length++] = (*messageid >> 8);
            buffer[length++] = (*messageid & 0xFF);
        }
        
        for (uint16_t i=0; i < plength && length < MQTT_MAX_PACKET_SIZE; i++) {
            buffer[length++] = payload[i];
        }
        
        uint8_t header = MQTTPUBLISH;
        if (kept) {
            header |= 1;
        }

        if (qos == QOS2)
            header |= MQTTQOS2_HEADER_MASK;
        else if (qos == QOS1)
            header |= MQTTQOS1_HEADER_MASK;
        else
            header |= MQTTQOS0_HEADER_MASK;

        return write(header, buffer, length-5);
    }
    return false;
}

bool MQTT::publishRelease(uint16_t messageid) {
    if (isConnected()) {
        uint16_t length = 0;
        buffer[length++] = MQTTPUBREL | MQTTQOS1_HEADER_MASK;
        buffer[length++] = 2;
        buffer[length++] = (messageid >> 8);
        buffer[length++] = (messageid & 0xFF);
        return _client.write(buffer, length);
    }
    return false;
}


bool MQTT::write(uint8_t header, uint8_t* buf, uint16_t length) {
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint8_t rc;
    uint8_t len = length;
    do {
        digit = len % 128;
        len = len / 128;
        if (len > 0) {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while(len > 0);

    buf[4-llen] = header;
    for (int i = 0; i < llen; i++) {
        buf[5-llen+i] = lenBuf[i];
    }
    rc = _client.write(buf+(4-llen), length+1+llen);
   
    lastOutActivity = millis();
    return (rc == 1+llen+length);
}

bool MQTT::subscribe(const char* topic) {
    return subscribe(topic, QOS0);
}

bool MQTT::subscribe(const char* topic, EMQTT_QOS qos) {
    if (qos < 0 || qos > 1)
        return false;

    if (isConnected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = 5;
        nextMsgId++;
        if (nextMsgId == 0) {
            nextMsgId = 1;
        }
        buffer[length++] = (nextMsgId >> 8);
        buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, buffer,length);
        buffer[length++] = qos;
        return write(MQTTSUBSCRIBE | MQTTQOS1_HEADER_MASK,buffer,length-5);
    }
    return false;
}

bool MQTT::unsubscribe(const char* topic) {
    if (isConnected()) {
        uint16_t length = 5;
        nextMsgId++;
        if (nextMsgId == 0) {
            nextMsgId = 1;
        }
        buffer[length++] = (nextMsgId >> 8);
        buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, buffer,length);
        return write(MQTTUNSUBSCRIBE | MQTTQOS1_HEADER_MASK,buffer,length-5);
    }
    return false;
}

void MQTT::disconnect() {
    buffer[0] = MQTTDISCONNECT;
    buffer[1] = 0;
    _client.write(buffer,2);
    _client.stop();
    lastInActivity = lastOutActivity = millis();
}

uint16_t MQTT::writeString(const char* string, uint8_t* buf, uint16_t pos) {
    const char* idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp && pos < MQTT_MAX_PACKET_SIZE) {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos-i-2] = (i >> 8);
    buf[pos-i-1] = (i & 0xFF);
    return pos;
}


bool MQTT::isConnected() {
    bool rc = (int)_client.connected();
    if (!rc) _client.stop();
    return rc;
}
