/*
 * File: SparkplugHost.h
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

#ifndef SRC_SPARKPLUGHOST
#define SRC_SPARKPLUGHOST

#include "MQTTAsync.h"
#include "types/Group.h"
#include "DataCollection.h"
#include <functional>
#include <map>
#include <atomic>
#include <memory>

using namespace std;

/**
 * @brief A class that acts as a Sparkplug Host on a MQTT network.
 * Monitors all Sparkplug Entities and ensures they are Sparkplug Complient.
 * Will trigger rebirths for Nodes who fail validation.
 *
 */
class SparkplugHost : public DataCollection<Group>
{
private:
    std::string server;
    std::string clientId;
    std::string hostId;
    std::string username;
    std::string password;

    mutex payloadLock;
    mutex commandLock;
    mutex receiverLock;
    std::vector<SparkplugMessage> commands;
    ParseResult process(SparkplugTopic &topic, tahu::Payload *payload);
    atomic<bool> running = false;

    std::unique_ptr<SparkplugReceiver> receiver;
    SparkplugReceiver *getReceiver();
    void buildReceiver();

protected:
public:
    /**
     * @brief Construct a new Sparkplug Host on a MQTT Server with a specific Client Id
     *
     * @param server
     * @param clientId
     */
    SparkplugHost(std::string server, std::string clientId);
    /**
     * @brief Construct a new Sparkplug Host on a MQTT Server with a specific Client Id as a Primary Host
     *
     * @param server
     * @param clientId
     */
    SparkplugHost(std::string server, std::string clientId, std::string hostId);
    /**
     * @brief Blocking Control loop.
     * Connects to MQTT Server and starts consuming messages.
     *
     * @return int
     */
    int run();
    /**
     * @brief Stops the main Control loop.
     *
     */
    void stop();
    /**
     * @brief Returns a list of changes since the last time this function was run.
     * Allows for only recent changes to be updated.
     *
     * @param force Forces the Sparkplug Host to return all data.
     * @return vector<PublishableUpdate>
     */
    vector<PublishableUpdate> getPayloads(bool force = false);

    /**
     * @brief Publishes a metric to a topic
     *
     * @param metric The metric and topic to publish
     * @return int
     */
    void command(SparkplugMessage message);

    /**
     * @brief Reconfigures the Host to connect to a new address
     *
     * @param address
     */
    void configure(std::string address);

    /**
     * @brief Enables and sets the Username and Password for the MQTT connection
     *
     * @param username
     * @param password
     */
    void credentials(std::string username, std::string password);
};

#endif /* SRC_SPARKPLUGHOST */
