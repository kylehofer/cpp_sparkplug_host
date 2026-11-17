/*
 * File: SparkplugReceiver.h
 * Project: cpp_sparkplug_host
 * Created Date: Tuesday December 19th 2023
 * Author: Kyle Hofer
 *
 * MIT License
 *
 * Copyright (c) 2024 Kyle Hofer
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

#ifndef SRC_SPARKPLUGRECEIVER
#define SRC_SPARKPLUGRECEIVER

#include "mqtt/async_client.h"
#include "mqtt/iaction_listener.h"
#include "SparkplugMessage.h"
#include "types/TahuTypes.h"
#include <chrono>
#include <string>

/**
 * @brief The MQTT connection used by SparkplugHost. Internal: not part of the public API.
 *
 * Subscribes to spBv1.0/#, decodes incoming payloads, publishes commands and rebirth requests,
 * and, when a host id is set, maintains the Primary Host STATE topic (birth, Will and
 * re-assertion when another client marks the host offline).
 *
 * Publishing methods never throw. They return false and log when the client is disconnected
 * or the broker rejects the message.
 */
class SparkplugReceiver : private mqtt::iaction_listener
{
private:
    /**
     * @brief Logs the result of the asynchronous connect
     */
    class ConnectListener : public mqtt::iaction_listener
    {
    public:
        void on_failure(const mqtt::token &token) override;
        void on_success(const mqtt::token &token) override;
    };

    std::string address;
    std::string clientId;
    std::string hostId;
    std::string username;
    std::string password;
    std::string hostIdTopic;
    std::string hostIdOffline;
    std::string hostIdOnline;
    uint64_t connectTime = 0;
    mqtt::ssl_options sslOptions;
    mqtt::connect_options connectionOptions;
    mqtt::async_client client;
    ConnectListener connectListener;

    bool publish(const std::string &topic, const std::string &payload, int qos, bool retained, bool wait = false);
    bool publishPayload(const std::string &topic, const tahu::Payload *payload);
    void handleState(const std::string &json);

    /** Called when the subscription fails */
    void on_failure(const mqtt::token &token) override;
    /** Called when the subscription succeeds. Publishes the online STATE. */
    void on_success(const mqtt::token &token) override;

public:
    /**
     * @brief Construct a new Sparkplug Receiver
     *
     * @param address The address of the mqtt server, e.g. tcp://localhost:1883 or ssl://broker:8883
     * @param clientId MQTT Client Id
     * @param hostId The Sparkplug Primary Host Id. Leave empty to run without a STATE topic.
     */
    SparkplugReceiver(const std::string &address, const std::string &clientId, const std::string &hostId = "");

    ~SparkplugReceiver();

    /**
     * @brief Sets the credentials used by configure(). Empty values are not sent.
     *
     * @param username
     * @param password
     */
    void credentials(const std::string &username, const std::string &password);

    /**
     * @brief Builds the connection options and registers the connection callbacks
     *
     * @return int 0
     */
    int configure();

    /**
     * @brief Starts connecting to the broker. Returns immediately; connection failures are logged
     * and retried automatically.
     *
     * @return int 0 if the connect was started, -1 if it was rejected
     */
    int activate();

    /**
     * @brief Takes the next Sparkplug message from the receive queue.
     * STATE messages for this host are handled internally and skipped, as are payloads that fail to decode.
     *
     * @param message Filled with the topic and decoded payload
     * @param wait How long to wait for a message if none is queued
     * @return true A message was returned
     * @return false No message arrived in time
     */
    bool consume(SparkplugMessage &message, std::chrono::milliseconds wait = std::chrono::milliseconds(0));

    /**
     * @brief Publishes a Node Control/Rebirth request
     *
     * @param topic The node's NCMD topic: spBv1.0/<group>/NCMD/<node>
     * @return true The request was published
     */
    bool rebirth(const std::string &topic);

    /**
     * @brief Publishes a command payload. Sets the payload timestamp and clears its sequence number.
     *
     * @param message
     * @return true The command was published
     */
    bool command(SparkplugMessage &message);

    /**
     * @brief Whether the client is currently connected
     */
    bool isConnected() const;

    /**
     * @brief Publishes the offline STATE (when a host id is set) and disconnects. Does not throw.
     *
     */
    void stop();
};

#endif /* SRC_SPARKPLUGRECEIVER */
