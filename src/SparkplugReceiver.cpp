/*
 * File: SparkplugReceiver.cpp
 * Project: cpp_sparkplug_host
 * Created Date: Tuesday December 19th 2023
 * Author: Kyle Hofer
 *
 * MIT License
 *
 * Copyright (c) 2023 Kyle Hofer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * HISTORY:
 */

#include "SparkplugReceiver.h"
#include "mqtt/message.h"
#include <string>
#include <chrono>

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...)    \
    printf("SparkplugHost: "); \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

using namespace std;
using namespace std::chrono;

using namespace std;
const string SPARKPLUG_ID{"spBv1.0"};
const string TOPIC{SPARKPLUG_ID + "/#"};

const int QOS = 1;

const mqtt::create_options createOptions(MQTTVERSION_5);

SparkplugReceiver::SparkplugReceiver(string address) : client(address, "", createOptions)
{
}
SparkplugReceiver::SparkplugReceiver(string address, string clientId) : client(address, clientId, createOptions)
{
    LOGGER("Configured to connect to %s.\n", address.c_str());
}

SparkplugReceiver::~SparkplugReceiver()
{
    client.stop_consuming();
    client.disable_callbacks();
    if (client.is_connected())
    {
        client.disconnect();
    }
}

int SparkplugReceiver::activate()
{
    client.start_consuming();

    auto token = client.connect(connectionOptions);

    return 0;
}

int SparkplugReceiver::configure()
{
    connectionOptions = mqtt::connect_options_builder()
                            .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, 604800}})
                            .clean_session(true)
                            .automatic_reconnect(seconds(1), seconds(10))
                            .finalize();

    client.set_connection_lost_handler([](const std::string &) {});

    client.set_disconnected_handler([](const mqtt::properties &, mqtt::ReasonCode reason) {});

    client.set_connected_handler([this](const std::string &)
                                 { client.subscribe(TOPIC, QOS, options); });
    return 0;
}

bool SparkplugReceiver::consume(SparkplugMessage &message)
{
    if (client.is_connected())
    {
        mqtt::const_message_ptr mqttMessage;
        if (client.try_consume_message(&mqttMessage))
        {
            mqtt::binary payload = mqttMessage->get_payload();

            // Decode the payload
            tahu::Payload *sparkplugPayload = (tahu::Payload *)malloc(sizeof(tahu::Payload));
            *sparkplugPayload = org_eclipse_tahu_protobuf_Payload_init_zero;
            if (decode_payload(sparkplugPayload, (uint8_t *)payload.data(), payload.length()) < 0)
            {
                free_payload(sparkplugPayload);
                free(sparkplugPayload);
                return false;
            }

            message.payload = sparkplugPayload;
            message.topic = mqttMessage->get_topic();
            return true;
        }
    }

    return false;
}

#define NODE_CONTROL_REBIRTH_NAME "Node Control/Rebirth"
#define DEVICE_CONTROL_REBIRTH_NAME "Device Control/Rebirth"

int SparkplugReceiver::rebirth(const std::string &topic)
{
    tahu::Payload *payload = (org_eclipse_tahu_protobuf_Payload *)malloc(sizeof(org_eclipse_tahu_protobuf_Payload));

    // Initialize payload
    memset(payload, 0, sizeof(org_eclipse_tahu_protobuf_Payload));
    payload->has_timestamp = true;
    payload->timestamp = get_current_timestamp();
    payload->has_seq = false;

    bool value = true;

    add_simple_metric(payload, NODE_CONTROL_REBIRTH_NAME, false, 0, METRIC_DATA_TYPE_BOOLEAN, false, false, &value, sizeof(value));

    size_t length = 2048;
    unsigned char *buffer = (uint8_t *)malloc(length * sizeof(uint8_t));
    size_t message_length = encode_payload(buffer, length, payload);

    LOGGER("Commanding a rebirth for %s.\n", topic.c_str());

    client.publish(mqtt::message::create(topic, buffer, message_length, 0, false));

    free_payload(payload);
    free(payload);
    return 0;
}

int SparkplugReceiver::command(tahu::Metric &metric, string topic)
{
    tahu::Payload *payload = (org_eclipse_tahu_protobuf_Payload *)malloc(sizeof(org_eclipse_tahu_protobuf_Payload));

    // Initialize payload
    memset(payload, 0, sizeof(org_eclipse_tahu_protobuf_Payload));
    payload->has_timestamp = true;
    payload->timestamp = get_current_timestamp();
    payload->has_seq = false;

    add_metric_to_payload(payload, &metric);

    size_t length = 2048;
    unsigned char *buffer = (uint8_t *)malloc(length * sizeof(uint8_t));
    size_t message_length = encode_payload(buffer, length, payload);

    client.publish(mqtt::message::create(topic, buffer, message_length, 0, false));

    free_payload(payload);
    free(payload);
    return 0;
}

int SparkplugReceiver::command(SparkplugMessage &message)
{
    // Initialize payload
    message.payload->has_timestamp = true;
    message.payload->timestamp = get_current_timestamp();
    message.payload->has_seq = false;

    size_t length = 2048;
    unsigned char *buffer = (uint8_t *)malloc(length * sizeof(uint8_t));
    size_t message_length = encode_payload(buffer, length, message.payload);

    client.publish(mqtt::message::create(message.topic, buffer, message_length, 0, false));
    return 0;
}
