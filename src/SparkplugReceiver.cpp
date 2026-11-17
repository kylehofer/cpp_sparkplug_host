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
#include "utilities/HostState.h"
#include "utilities/LogMacros.h"
#include <cstring>
#include <string>

namespace
{
    const char *const COMPONENT = "SparkplugReceiver";
    const std::string SPARKPLUG_ID{"spBv1.0"};
    const std::string SPARKPLUG_TOPIC{SPARKPLUG_ID + "/#"};
    const char *const NODE_CONTROL_REBIRTH_NAME = "Node Control/Rebirth";

    const auto PUBLISH_TIMEOUT = std::chrono::seconds(2);
    const auto DISCONNECT_TIMEOUT = std::chrono::seconds(5);

    bool startsWith(const std::string &value, const char *prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }

    bool usesSsl(const std::string &address)
    {
        return startsWith(address, "ssl://") || startsWith(address, "mqtts://") || startsWith(address, "wss://");
    }
}

void SparkplugReceiver::ConnectListener::on_failure(const mqtt::token &token)
{
    SPH_LOG_ERROR(COMPONENT, "Connection attempt failed (code {}, reason {}). Retrying automatically.",
                  token.get_return_code(), static_cast<int>(token.get_reason_code()));
}

void SparkplugReceiver::ConnectListener::on_success(const mqtt::token &)
{
}

SparkplugReceiver::SparkplugReceiver(const std::string &address, const std::string &clientId, const std::string &hostId)
    : address(address),
      clientId(clientId),
      hostId(hostId),
      client(address, clientId, mqtt::create_options(MQTTVERSION_5))
{
    SPH_LOG_INFO(COMPONENT, "Configured to connect to {}", address);
}

SparkplugReceiver::~SparkplugReceiver()
{
    try
    {
        client.disable_callbacks();
        client.stop_consuming();
        if (client.is_connected())
        {
            client.disconnect()->wait_for(DISCONNECT_TIMEOUT);
        }
    }
    catch (const std::exception &error)
    {
        SPH_LOG_WARNING(COMPONENT, "Error while closing the connection: {}", error.what());
    }
}

void SparkplugReceiver::credentials(const std::string &username, const std::string &password)
{
    this->username = username;
    this->password = password;
}

int SparkplugReceiver::configure()
{
    // MQTT 5 requires Clean Start with a Session Expiry of 0 for Sparkplug Host Applications
    auto connectionBuilder = mqtt::connect_options_builder::v5()
                                 .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, 0}})
                                 .clean_start(true)
                                 .automatic_reconnect(std::chrono::seconds(1), std::chrono::seconds(10));

    if (usesSsl(address))
    {
        SPH_LOG_INFO(COMPONENT, "Enabling SSL");
        connectionBuilder.ssl(sslOptions);
    }

    if (!username.empty())
    {
        SPH_LOG_INFO(COMPONENT, "Connecting with the username: {}", username);
        connectionBuilder.user_name(username);
    }

    if (!password.empty())
    {
        connectionBuilder.password(password);
    }

    if (!hostId.empty())
    {
        SPH_LOG_INFO(COMPONENT, "Configuring with the Host ID: {}", hostId);
        // The Will and the online STATE must carry the same timestamp. The Will is fixed
        // for the life of these options, so every automatic reconnect reuses it.
        connectTime = get_current_timestamp();
        hostIdTopic = SPARKPLUG_ID + "/STATE/" + hostId;
        hostIdOffline = HostState{false, connectTime}.toJson();
        hostIdOnline = HostState{true, connectTime}.toJson();

        connectionBuilder.will(mqtt::will_options(hostIdTopic, hostIdOffline, 1, true));
    }

    connectionOptions = connectionBuilder.finalize();

    client.set_connection_lost_handler(
        [](const std::string &cause)
        { SPH_LOG_WARNING(COMPONENT, "Connection lost: {}", cause); });

    client.set_disconnected_handler(
        [](const mqtt::properties &, mqtt::ReasonCode reason)
        { SPH_LOG_WARNING(COMPONENT, "Disconnected by the broker (reason {})", static_cast<int>(reason)); });

    client.set_connected_handler(
        [this](const std::string &)
        {
            SPH_LOG_INFO(COMPONENT, "Connected to {}", address);
            std::vector<std::string> subscribeTopics = {SPARKPLUG_TOPIC};
            mqtt::iasync_client::qos_collection subscribeQos{0};

            if (!hostId.empty())
            {
                // Sparkplug requires the host to subscribe to its own STATE topic at QoS 1
                subscribeTopics.push_back(hostIdTopic);
                subscribeQos.push_back(1);
            }

            try
            {
                auto token = client.subscribe(mqtt::string_collection::create(subscribeTopics), subscribeQos);
                token->set_action_callback(*this);
            }
            catch (const std::exception &error)
            {
                SPH_LOG_ERROR(COMPONENT, "Subscribe failed: {}", error.what());
            }
        });

    return 0;
}

int SparkplugReceiver::activate()
{
    try
    {
        client.start_consuming();
        client.connect(connectionOptions, nullptr, connectListener);
    }
    catch (const std::exception &error)
    {
        SPH_LOG_ERROR(COMPONENT, "Unable to start connecting to {}: {}", address, error.what());
        return -1;
    }
    return 0;
}

bool SparkplugReceiver::consume(SparkplugMessage &message, std::chrono::milliseconds wait)
{
    auto deadline = std::chrono::steady_clock::now() + wait;

    while (true)
    {
        mqtt::const_message_ptr mqttMessage;
        bool received = wait.count() > 0
                            ? client.try_consume_message_until(&mqttMessage, deadline)
                            : client.try_consume_message(&mqttMessage);

        if (!received)
        {
            return false;
        }

        if (!mqttMessage)
        {
            continue;
        }

        const std::string &topic = mqttMessage->get_topic();

        if (!hostId.empty() && topic == hostIdTopic)
        {
            handleState(mqttMessage->get_payload_str());
            continue;
        }

        if (startsWith(topic, (SPARKPLUG_ID + "/STATE/").c_str()))
        {
            // Another host's STATE message. Not a protobuf payload.
            continue;
        }

        const mqtt::binary_ref &payload = mqttMessage->get_payload_ref();
        tahu::PayloadPtr decoded = tahu::decode(reinterpret_cast<const uint8_t *>(payload.data()), payload.size());
        if (!decoded)
        {
            SPH_LOG_WARNING(COMPONENT, "Unable to decode the payload on {}", topic);
            continue;
        }

        message.topic = topic;
        message.payload = std::move(decoded);
        return true;
    }
}

void SparkplugReceiver::handleState(const std::string &json)
{
    auto state = HostState::parse(json);
    if (!state)
    {
        SPH_LOG_WARNING(COMPONENT, "Ignoring an invalid STATE payload on {}", hostIdTopic);
        return;
    }

    if (!state->online)
    {
        // Another client (or a stale Will) marked this host offline while it is running
        SPH_LOG_INFO(COMPONENT, "Re-publishing the online STATE");
        publish(hostIdTopic, hostIdOnline, 1, true);
    }
}

bool SparkplugReceiver::publish(const std::string &topic, const std::string &payload, int qos, bool retained, bool wait)
{
    if (!client.is_connected())
    {
        SPH_LOG_WARNING(COMPONENT, "Not connected. Dropping the message for {}", topic);
        return false;
    }

    try
    {
        auto token = client.publish(mqtt::message::create(topic, payload, qos, retained));
        if (wait && !token->wait_for(PUBLISH_TIMEOUT))
        {
            SPH_LOG_WARNING(COMPONENT, "Timed out publishing to {}", topic);
            return false;
        }
        return true;
    }
    catch (const std::exception &error)
    {
        SPH_LOG_ERROR(COMPONENT, "Publishing to {} failed: {}", topic, error.what());
        return false;
    }
}

bool SparkplugReceiver::publishPayload(const std::string &topic, const tahu::Payload *payload)
{
    auto encoded = tahu::encode(payload);
    if (!encoded)
    {
        SPH_LOG_ERROR(COMPONENT, "Unable to encode the payload for {}", topic);
        return false;
    }

    return publish(topic, std::string(encoded->begin(), encoded->end()), 0, false);
}

bool SparkplugReceiver::rebirth(const std::string &topic)
{
    tahu::PayloadPtr payload = tahu::makePayload();
    payload->has_timestamp = true;
    payload->timestamp = get_current_timestamp();
    payload->has_seq = false;

    bool value = true;
    add_simple_metric(payload.get(), NODE_CONTROL_REBIRTH_NAME, false, 0, METRIC_DATA_TYPE_BOOLEAN, false, false, &value, sizeof(value));

    SPH_LOG_INFO(COMPONENT, "Requesting a rebirth on {}", topic);
    return publishPayload(topic, payload.get());
}

bool SparkplugReceiver::command(SparkplugMessage &message)
{
    if (!message.payload)
    {
        SPH_LOG_WARNING(COMPONENT, "Ignoring a command without a payload for {}", message.topic);
        return false;
    }

    message.payload->has_timestamp = true;
    message.payload->timestamp = get_current_timestamp();
    message.payload->has_seq = false;

    return publishPayload(message.topic, message.payload.get());
}

bool SparkplugReceiver::isConnected() const
{
    return client.is_connected();
}

void SparkplugReceiver::on_failure(const mqtt::token &token)
{
    SPH_LOG_ERROR(COMPONENT, "Subscribing failed (code {}). No Sparkplug messages will be received.",
                  token.get_return_code());
}

void SparkplugReceiver::on_success(const mqtt::token &)
{
    SPH_LOG_INFO(COMPONENT, "Subscribed");
    if (!hostId.empty())
    {
        publish(hostIdTopic, hostIdOnline, 1, true);
    }
}

void SparkplugReceiver::stop()
{
    try
    {
        if (!hostId.empty() && client.is_connected())
        {
            publish(hostIdTopic, hostIdOffline, 1, true, true);
        }
        if (client.is_connected())
        {
            client.disconnect()->wait_for(DISCONNECT_TIMEOUT);
        }
    }
    catch (const std::exception &error)
    {
        SPH_LOG_WARNING(COMPONENT, "Error while disconnecting: {}", error.what());
    }
}
