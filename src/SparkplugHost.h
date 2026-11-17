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

#include "DataCollection.h"
#include "SparkplugMessage.h"
#include "types/Group.h"
#include "types/PublishableUpdate.h"
#include "types/TahuTypes.h"
#include "utilities/Logger.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class SparkplugReceiver;

/**
 * @brief A Sparkplug B (3.0) Host Application.
 *
 * Connects to an MQTT broker, tracks every Group, Node, Device and Metric it sees, validates
 * sequence numbers and requests rebirths from Nodes that fall out of sync. When constructed with a
 * host id it acts as the Primary Host and maintains spBv1.0/STATE/<host_id>.
 *
 * The host is driven by a polling loop: run() blocks on its own thread, and the application calls
 * getPayloads() periodically to collect what changed.
 *
 * @par Thread safety
 * run() must only be running on one thread at a time. Every other member function may be called
 * from any thread, including while run() is executing.
 *
 * @code
 * SparkplugHost host("tcp://localhost:1883", "my-host-client", "my-host");
 * std::thread loop([&host] { host.run(); });
 *
 * for (auto &update : host.getPayloads()) { ... }
 *
 * host.stop();
 * loop.join();
 * @endcode
 */
class SparkplugHost : private DataCollection<Group>
{
private:
    std::string server;
    std::string clientId;
    std::string hostId;
    std::string username;
    std::string password;

    std::mutex payloadLock;
    std::mutex commandLock;
    std::mutex configLock;
    std::deque<SparkplugMessage> commands;
    std::atomic<bool> running = false;
    std::atomic<bool> connected = false;
    bool reconfigure = false;

    std::unique_ptr<SparkplugReceiver> receiver;

    ParseResult process(const SparkplugTopic &topic, const tahu::Payload *payload);
    void buildReceiver();
    void markAllOffline();
    void loop(bool &wasConnected);

public:
    /**
     * @brief Construct a new Sparkplug Host. Nothing connects until run() is called.
     *
     * @param server Broker address, e.g. tcp://localhost:1883. Use ssl:// or mqtts:// for TLS.
     * @param clientId MQTT client id. Must be unique on the broker.
     * @param hostId Sparkplug Host Id. When set, the host publishes spBv1.0/STATE/<hostId> and acts
     *               as the Primary Host. Leave empty for a passive host.
     */
    SparkplugHost(std::string server, std::string clientId, std::string hostId = "");
    ~SparkplugHost();

    SparkplugHost(const SparkplugHost &) = delete;
    SparkplugHost &operator=(const SparkplugHost &) = delete;

    /**
     * @brief Blocking control loop.
     * Connects to the MQTT server, processes incoming messages and sends queued commands until
     * stop() is called. On return the host has published its offline STATE and disconnected.
     *
     * @return int 0
     */
    int run();
    /**
     * @brief Asks run() to finish. Returns immediately; run() returns within about 100 ms.
     *
     */
    void stop();
    /**
     * @brief Returns what changed since the last call.
     *
     * Each Node and Device that changed produces at most one update:
     * - BIRTH with every metric, after the entity was birthed
     * - PUBLISH with only the metrics that changed
     * - DEATH, with no payload, after the entity went offline
     *
     * @param force Return a complete snapshot instead: a BIRTH for every online entity and a DEATH
     *              for every offline one. A forced call does not consume pending changes, so the next
     *              normal call still reports them.
     * @return std::vector<PublishableUpdate> Owned updates; payloads are freed with the updates.
     */
    std::vector<PublishableUpdate> getPayloads(bool force = false);

    /**
     * @brief Queues a payload to be published on a topic. Sent on the next loop iteration
     * while connected; commands queued while disconnected wait until the connection returns.
     *
     * The payload timestamp is set when it is sent and its sequence number is cleared.
     *
     * @param message Takes ownership of the payload
     */
    void command(SparkplugMessage message);

    /**
     * @brief Queues an NCMD to a Node: spBv1.0/<group>/NCMD/<node>
     *
     * @param group
     * @param node
     * @param payload Command metrics. Ownership is taken.
     */
    void commandNode(const std::string &group, const std::string &node, tahu::PayloadPtr payload);

    /**
     * @brief Queues a DCMD to a Device: spBv1.0/<group>/DCMD/<node>/<device>
     *
     * @param group
     * @param node
     * @param device
     * @param payload Command metrics. Ownership is taken.
     */
    void commandDevice(const std::string &group, const std::string &node, const std::string &device, tahu::PayloadPtr payload);

    /**
     * @brief Queues a Node Control/Rebirth request to a Node
     *
     * @param group
     * @param node
     */
    void requestRebirth(const std::string &group, const std::string &node);

    /**
     * @brief Changes the broker address. Takes effect on the next loop iteration: the host
     * disconnects, forgets all tracked state and reconnects.
     *
     * @param address
     */
    void configure(std::string address);

    /**
     * @brief Sets the username and password for the MQTT connection. Takes effect on the next loop
     * iteration in the same way as configure(). Empty values are not sent.
     *
     * @param username
     * @param password
     */
    void credentials(std::string username, std::string password);

    /**
     * @brief Whether the host is currently connected to the broker
     *
     * @return true
     * @return false
     */
    bool isConnected() const;

    /**
     * @brief Sets how long to wait for an NBIRTH before asking a Node to rebirth again.
     * Defaults to 5 seconds. Applies to all hosts in the process.
     *
     * @param interval
     */
    static void setRebirthRetryInterval(std::chrono::milliseconds interval);
};

#endif /* SRC_SPARKPLUGHOST */
