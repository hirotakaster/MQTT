#include "MQTT.h"
#include "ISubCallback.h"

/*
    This methodology allows the mqtt driver to exist in non-global space (such as belonging to another wrapper class).
    Other features here allow default constructor, with post constructed Initialization (but only a single time).
    Also, the callback feature is backwards compatible with a global function callback OR the option of registering
    other objects (by means of the ISubCallback interface) to a list of callback listeners.
*/

class Foo : ISubCallback{
public:
    Foo() {
        client.Initialize("some.domain.com", NULL, 1883, MQTT_DEFAULT_KEEPALIVE, MQTT_MAX_PACKET_SIZE, NULL, true);
        client.RegisterCallbackListener(this);
    }

    void Update() {
        if (client.isConnected())
            client.publish("outTopic/message","hello world");
            client.loop();
        else {
            client.connect("sparkclient");
            client.subscribe("inTopic/message");
        }
    }

    void Callback(char* topic, byte* payload, unsigned int length) {
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
private:
    MQTT client;
}

Foo foo;


void setup() {
    RGB.control(true);
}

void loop() {
    foo.Update();
}
