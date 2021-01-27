#include "MQTT.h"

#define LOGGING

#define MQTTQOS0_HEADER_MASK        (0 << 1)
#define MQTTQOS1_HEADER_MASK        (1 << 1)
#define MQTTQOS2_HEADER_MASK        (2 << 1)

#define DUP_FLAG_OFF_MASK           (0<<3)
#define DUP_FLAG_ON_MASK            (1<<3)

MQTT::MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(domain, NULL, port, MQTT_DEFAULT_KEEPALIVE, MQTT_MAX_PACKET_SIZE, callback, thread);
}

MQTT::MQTT(char* domain, uint16_t port, int maxpacketsize, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(domain, NULL, port, MQTT_DEFAULT_KEEPALIVE, maxpacketsize, callback, thread);
}

MQTT::MQTT(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(NULL, ip, port, MQTT_DEFAULT_KEEPALIVE, MQTT_MAX_PACKET_SIZE, callback, thread);
}

MQTT::MQTT(uint8_t *ip, uint16_t port, int maxpacketsize, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(NULL, ip, port, MQTT_DEFAULT_KEEPALIVE, maxpacketsize, callback, thread);
}

MQTT::MQTT(char* domain, uint16_t port, int maxpacketsize, int keepalive, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(domain, NULL, port, keepalive, maxpacketsize, callback, thread);
}

MQTT::MQTT(uint8_t *ip, uint16_t port, int maxpacketsize, int keepalive, void (*callback)(char*,uint8_t*,unsigned int),
            bool thread) {
    this->initialize(NULL, ip, port, keepalive, maxpacketsize, callback, thread);
}

MQTT::~MQTT() {
    if (isConnected()) {
        disconnect();
    }

    if (buffer != NULL)
      delete[] buffer;
}

void MQTT::initialize(char* domain, uint8_t *ip, uint16_t port, int keepalive, int maxpacketsize, 
                    void (*callback)(char*,uint8_t*,unsigned int), bool thread) {
    if (thread) {
        this->thread = true;
        os_mutex_create(&mutex_lock);
    }
    this->callback = callback;
    this->qoscallback = NULL;
    if (ip != NULL)
        this->ip = ip;
    if (domain != NULL)
        this->domain = domain;
    this->port = port;
    this->keepalive = keepalive;

    // if maxpacketsize is over MQTT_MAX_PACKET_SIZE.
    this->maxpacketsize = (maxpacketsize <= MQTT_MAX_PACKET_SIZE ? MQTT_MAX_PACKET_SIZE : maxpacketsize);
    if (buffer != NULL)
      delete[] buffer;
    buffer = new uint8_t[this->maxpacketsize];
}

void MQTT::setBroker(char* domain, uint16_t port) {
    if(isConnected()) {
        disconnect();
    }
    this->domain = domain;
    this->ip = NULL;
    this->port = port;
}

void MQTT::setBroker(uint8_t *ip, uint16_t port) {
    if(isConnected()) {
        disconnect();
    }
    this->domain = "";
    this->ip = ip;
    this->port = port;
}


void MQTT::addQosCallback(void (*qoscallback)(unsigned int)) {
    this->qoscallback = qoscallback;
}


bool MQTT::connect(const char *id) {
    return connect(id, NULL, NULL, 0, QOS0, 0, 0, true);
}

bool MQTT::connect(const char *id, const char *user, const char *pass) {
    return connect(id, user, pass, 0, QOS0, 0, 0, true);
}

bool MQTT::connect(const char *id, const char *user, const char *pass, const char* willTopic, EMQTT_QOS willQos, uint8_t willRetain, const char* willMessage, bool cleanSession, MQTT_VERSION version) {
    if (!isConnected()) {
        MutexLocker lock(this);
        int result = 0;
        if (ip == NULL)
            result = _client.connect(this->domain.c_str(), this->port);
        else
            result = _client.connect(this->ip, this->port);

        if (result) {
            nextMsgId = 1;
            uint16_t length = 5;

            if (version == MQTT_V311) {
                const uint8_t MQTT_HEADER_V311[] = {0x00,0x04,'M','Q','T','T',MQTT_V311};
                memcpy(buffer + length, MQTT_HEADER_V311, sizeof(MQTT_HEADER_V311));
                length+=sizeof(MQTT_HEADER_V311);
            } else {
                const uint8_t MQTT_HEADER_V31[] = {0x00,0x06,'M','Q','I','s','d','p', MQTT_V31};
                memcpy(buffer + length, MQTT_HEADER_V31, sizeof(MQTT_HEADER_V31));
                length+=sizeof(MQTT_HEADER_V31);
            }

            uint8_t v = 0;
            if (willTopic) {
                v = 0x06|(willQos<<3)|(willRetain<<5);
            } else {
                v = 0x02;
            }

            if (!cleanSession) {
              v = v&0xfd;
            }

            if(user != NULL) {
                v = v|0x80;

                if(pass != NULL) {
                    v = v|(0x80>>1);
                }
            }

            buffer[length++] = v;

            buffer[length++] = ((this->keepalive) >> 8);
            buffer[length++] = ((this->keepalive) & 0xFF);
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
                if (t-lastInActivity > this->keepalive*1000UL) {
                    _client.stop();
                    return false;
                }
            }
            uint8_t llen;
            uint16_t len = readPacket(&llen);

            if (len == 4) {
                if (buffer[3] == CONN_ACCEPT) {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    debug_print(" Connect success\n");
                    return true;
                } else {
                    // check EMQTT_CONNACK_RESPONSE code.
                    debug_print(" Connect fail. code = [%d]\n", buffer[3]);
                }
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
        if (len < this->maxpacketsize) {
            buffer[len] = digit;
        }
        len++;
    }

    if (len > this->maxpacketsize) {
        len = 0; // This will cause the packet to be ignored.
    }

    return len;
}

bool MQTT::loop() {
    if (isConnected()) {
        MutexLocker lock(this);

        unsigned long t = millis();
        if ((t - lastInActivity > this->keepalive*1000UL) || (t - lastOutActivity > this->keepalive*1000UL)) {
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
                        uint16_t tl = (buffer[llen+1]<<8)+buffer[llen+2]; // topic length
                        char topic[tl+1];
                        for (uint16_t i=0;i<tl;i++) {
                            topic[i] = buffer[llen+3+i];
                        }
                        topic[tl] = 0;
                        // msgId only present for QOS>0
                        if ((buffer[0]&0x06) == MQTTQOS1_HEADER_MASK) { // QoS=1
                            msgId = (buffer[llen+3+tl]<<8)+buffer[llen+3+tl+1];
                            payload = buffer+llen+3+tl+2;
                            callback(topic,payload,len-llen-3-tl-2);

                            buffer[0] = MQTTPUBACK; // respond with PUBACK
                            buffer[1] = 2;
                            buffer[2] = (msgId >> 8);
                            buffer[3] = (msgId & 0xFF);
                            _client.write(buffer,4);
                            lastOutActivity = t;
                                    } else if ((buffer[0] & 0x06) == MQTTQOS2_HEADER_MASK) { // QoS=2
                                        msgId = (buffer[llen + 3 + tl] << 8) + buffer[llen + 3 + tl + 1];
                                        payload = buffer + llen + 3 + tl + 2;
                                        callback(topic, payload, len - llen - 3 - tl - 2);

                                        buffer[0] = MQTTPUBREC; // respond with PUBREC
                                        buffer[1] = 2;
                                        buffer[2] = (msgId >> 8);
                                        buffer[3] = (msgId & 0xFF);
                                        _client.write(buffer, 4);
                                        lastOutActivity = t;
                                    } else {
                            payload = buffer+llen+3+tl;
                            callback(topic,payload,len-llen-3-tl);
                        }
                    }
                } else if (type == MQTTPUBREC) {
                    // check for the situation that QoS2 receive PUBREC, should return PUBREL
                    msgId = (buffer[2] << 8) + buffer[3];
                    this->publishRelease(msgId);
                } else if (type == MQTTPUBACK) {
                    if (qoscallback) {
                        // this case QOS==1
                        if (len == 4 && (buffer[0]&0x06) == MQTTQOS0_HEADER_MASK) {
                            msgId = (buffer[2]<<8)+buffer[3];
                            this->qoscallback(msgId);
                        }
                    }
                } else if (type == MQTTPUBREL) {
                msgId = (buffer[2] << 8) + buffer[3];
                this->publishComplete(msgId);
                } else if (type == MQTTPUBCOMP) {
                if (qoscallback) {
                    // msgId only present for QOS==0
                    if (len == 4 && (buffer[0]&0x06) == MQTTQOS0_HEADER_MASK) {
                        msgId = (buffer[2]<<8)+buffer[3];
                        this->qoscallback(msgId);
                    }
                }
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

bool MQTT::publish(const char* topic, const char* payload, bool retain) {
    return publish(topic, (uint8_t*)payload, strlen(payload), retain, QOS0, NULL);
}

bool MQTT::publish(const char * topic, const char* payload, EMQTT_QOS qos, bool dup, uint16_t *messageid) {
    return publish(topic, (uint8_t*)payload, strlen(payload), false, qos, dup, messageid);
}

bool MQTT::publish(const char * topic, const char* payload, EMQTT_QOS qos, uint16_t *messageid) {
    return publish(topic, (uint8_t*)payload, strlen(payload), false, qos, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength) {
    return publish(topic, payload, plength, false, QOS0, NULL);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, EMQTT_QOS qos, bool dup, uint16_t *messageid) {
    return publish(topic, payload, plength, false, qos, dup, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, EMQTT_QOS qos, uint16_t *messageid) {
    return publish(topic, payload, plength, false, qos, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool retain) {
    return publish(topic, payload, plength, retain, QOS0, NULL);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool retain, EMQTT_QOS qos, uint16_t *messageid) {
    return publish(topic, payload, plength, retain, qos, false, messageid);
}

bool MQTT::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool retain, EMQTT_QOS qos, bool dup, uint16_t *messageid) {
    if (isConnected()) {
        MutexLocker lock(this);
        // Leave room in the buffer for header and variable length field
        uint16_t length = 5;
        memset(buffer, 0, this->maxpacketsize);

        length = writeString(topic, buffer, length);

        if (qos == QOS2 || qos == QOS1) {
            nextMsgId += 1;
            buffer[length++] = (nextMsgId >> 8);
            buffer[length++] = (nextMsgId & 0xFF);
            if (messageid != NULL)
                *messageid = nextMsgId++;
        }

        for (uint16_t i=0; i < plength && length < this->maxpacketsize; i++) {
            buffer[length++] = payload[i];
        }

        uint8_t header = MQTTPUBLISH;
        if (retain) {
            header |= 1;
        }

        if (dup) {
            header |= DUP_FLAG_ON_MASK;
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
        MutexLocker lock(this);
        uint16_t length = 0;
        // reserved bits in MQTT v3.1.1
        buffer[length++] = MQTTPUBREL | MQTTQOS1_HEADER_MASK;
        buffer[length++] = 2;
        buffer[length++] = (messageid >> 8);
        buffer[length++] = (messageid & 0xFF);
        return _client.write(buffer, length);
    }
    return false;
}

bool MQTT::publishComplete(uint16_t messageid) {
    if (isConnected()) {
        MutexLocker lock(this);
        uint16_t length = 0;
        // reserved bits in MQTT v3.1.1
        buffer[length++] = MQTTPUBCOMP | MQTTQOS1_HEADER_MASK;
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
    uint16_t rc;
    uint16_t len = length;
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

    if (isConnected()) {
        // Leave room in the buffer for header and variable length field
        MutexLocker lock(this);
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
        MutexLocker lock(this);
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
    MutexLocker lock(this);
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
    while (*idp && pos < this->maxpacketsize) {
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

void MQTT::clear() {
  _client.stop();
  lastInActivity = lastOutActivity = millis();
}
