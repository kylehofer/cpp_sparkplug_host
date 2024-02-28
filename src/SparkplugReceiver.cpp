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
#include "mqtt/string_collection.h"
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
const string SPARKPLUG_TOPIC{SPARKPLUG_ID + "/#"};

const int QOS = 0;

const mqtt::create_options createOptions(MQTTVERSION_5);

SparkplugReceiver::SparkplugReceiver(string address) : client(address, "", createOptions)
{
    if (address.find("ssl://") != std::string::npos)
    {
        useSsl = true;
    }
}
SparkplugReceiver::SparkplugReceiver(string address, string clientId) : client(address, clientId, createOptions)
{
    LOGGER("Configured to connect to %s.\n", address.c_str());
    if (address.find("ssl://") != std::string::npos)
    {
        useSsl = true;
    }
}

SparkplugReceiver::SparkplugReceiver(string address, string clientId, string hostId) : client(address, clientId, createOptions), hostId(hostId)
{
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
    auto connectionBuilder = mqtt::connect_options_builder()
                                 .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, 0}})
                                 .clean_start(true)
                                 .clean_session(true)
                                 .automatic_reconnect(seconds(1), seconds(10));

    if (useSsl)
    {
        connectionBuilder.ssl(sslOptions);
    }

    if (!username.empty())
    {
        mqtt::connect_options_builder().user_name(username.c_str());
    }

    if (!password.empty())
    {
        mqtt::connect_options_builder().password(password.c_str());
    }

    if (!hostId.empty())
    {
        connectTime = get_current_timestamp();
        hostIdTopic = {SPARKPLUG_ID + "/STATE/" + hostId};

        hostIdOffline = {"{ \"online\": false, \"timestamp\": " + std::to_string(connectTime) + " }"};
        hostIdOnline = {"{ \"online\": true, \"timestamp\": " + std::to_string(connectTime) + " }"};

        will = mqtt::will_options(hostIdTopic, hostIdOffline, 1, true);
        connectionBuilder = connectionBuilder.will(will);
    }

    connectionOptions = connectionBuilder.finalize();

    client.set_connection_lost_handler([](const std::string &) {});

    client.set_disconnected_handler([](const mqtt::properties &, mqtt::ReasonCode reason) {});

    client.set_connected_handler(
        [this](const std::string &)
        {
            std::vector<std::string> subscribeTopics = {SPARKPLUG_TOPIC};
            mqtt::iasync_client::qos_collection subscribeQos{0};

            if (!hostId.empty())
            {
                subscribeTopics.push_back(hostIdTopic);
                subscribeQos.push_back(0);
            }

            mqtt::const_string_collection_ptr subscribeTopicsPtr{mqtt::string_collection::create(subscribeTopics)};

            auto token = client.subscribe(subscribeTopicsPtr, subscribeQos);
            token->set_action_callback(*this);
        });

    return 0;
}

bool SparkplugReceiver::consume(SparkplugMessage &message)
{
    if (client.is_connected())
    {
        mqtt::const_message_ptr mqttMessage;
        if (client.try_consume_message(&mqttMessage))
        {
            if (mqttMessage->get_topic().compare(hostIdTopic) == 0)
            {
                auto json = mqttMessage->get_payload_str();

                if (json.find("\"online\": false") != std::string::npos)
                {
                    client.publish(mqtt::message::create(hostIdTopic, hostIdOnline, 1, true));
                }

                return false;
            }

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

    free(buffer);
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
    free(buffer);
    return 0;
}

void SparkplugReceiver::on_failure(const mqtt::token &asyncActionToken)
{
}

void SparkplugReceiver::on_success(const mqtt::token &asyncActionToken)
{
    if (hostId.empty())
    {
        return;
    }

    client.publish(mqtt::message::create(hostIdTopic, hostIdOnline, 1, true));
}

void SparkplugReceiver::stop()
{
    if (!hostId.empty())
    {
        client.publish(mqtt::message::create(hostIdTopic, hostIdOffline, 1, true))->wait();
    }
    if (client.is_connected())
    {

        client.disconnect()->wait();
    }
}

void SparkplugReceiver::credentials(std::string username, std::string password)
{
    this->username = username;
    this->password = password;
}
