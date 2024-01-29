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
#include "types/TahuTypes.h"
#include "utilities/SparkplugTopic.h"

using namespace std;

/**
 * @brief Struct for holding the data from a Sparkplug Payload along with the topic is published on
 *
 */
struct SparkplugMessage
{
public:
    SparkplugMessage(){};
    SparkplugMessage(string topic, tahu::Payload *payload) : topic(topic), payload(payload){};
    tahu::Payload *payload;
    string topic;
};

/**
 * @brief A Mqtt Client for both sending and receiving sparkplug payloads
 *
 */
class SparkplugReceiver
{
private:
    mqtt::connect_options connectionOptions;
    mqtt::async_client client;
    const mqtt::subscribe_options options = mqtt::subscribe_options(
        mqtt::subscribe_options::SUBSCRIBE_NO_LOCAL,
        false,
        mqtt::subscribe_options::DONT_SEND_RETAINED);

protected:
public:
    /**
     * @brief Construct a new Sparkplug Receiver
     *
     * @param address The address of the mqtt server to listen to
     */
    SparkplugReceiver(string address);
    /**
     * @brief Construct a new Sparkplug Receiver
     *
     * @param address The address of the mqtt server to listen to
     * @param clientId MQTT Client Id
     */
    SparkplugReceiver(string address, string clientId);

    /**
     * @brief Activates the receiver to start receiving messages
     *
     * @return int
     */
    int activate();

    /**
     * @brief Configures the MQTT client and event callbacks
     *
     * @return int
     */
    int configure();

    /**
     * @brief Attempts to consume a Sparkplug payload from the MQTT Client.
     *
     * @param message A reference to a message which will be filled with data
     * @return true If a message was consumed
     * @return false No message was consumed
     */
    bool consume(SparkplugMessage &message);

    /**
     * @brief Publishes a Sparkplug payload with a Node Rebirth Metric
     *
     * @param topic The topic to post the Rebirth Metric to
     * @return int
     */
    int rebirth(const std::string &topic);
    /**
     * @brief Publishes a metric to a topic
     *
     * @param metric The metric to publish
     * @param topic The topic to publish the metric on
     * @return int
     */
    int command(tahu::Metric &metric, string topic);
    /**
     * @brief Publishes a metric to a topic
     *
     * @param metric The metric and topic to publish
     * @return int
     */
    int command(SparkplugMessage &message);
};

#endif /* SRC_SPARKPLUGRECEIVER */
