/*
 * File: SparkplugHost.cpp
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

#include "SparkplugHost.h"
#include "SparkplugReceiver.h"
#include "utilities/LogMacros.h"
#include "utilities/SparkplugTopic.h"
#include <set>
#include <thread>

namespace
{
    const char *const COMPONENT = "SparkplugHost";
    const std::string SPARKPLUG_ID{"spBv1.0"};
    const auto RECEIVE_WAIT = std::chrono::milliseconds(100);
    const auto ERROR_BACKOFF = std::chrono::milliseconds(500);
    // Upper bound on messages handled per iteration, so commands and stop() are not starved
    const size_t MAX_MESSAGES_PER_ITERATION = 1000;

    std::string nodeCommandTopic(const std::string &group, const std::string &node)
    {
        return SPARKPLUG_ID + "/" + group + "/NCMD/" + node;
    }
}

SparkplugHost::SparkplugHost(std::string server, std::string clientId, std::string hostId)
    : server(std::move(server)), clientId(std::move(clientId)), hostId(std::move(hostId))
{
}

SparkplugHost::~SparkplugHost()
{
    running = false;
    if (receiver)
    {
        receiver->stop();
    }
}

int SparkplugHost::run()
{
    running = true;
    bool wasConnected = false;

    while (running)
    {
        try
        {
            loop(wasConnected);
        }
        catch (const std::exception &error)
        {
            SPH_LOG_ERROR(COMPONENT, "Unexpected error in the control loop: {}", error.what());
            std::this_thread::sleep_for(ERROR_BACKOFF);
        }
    }

    if (receiver)
    {
        receiver->stop();
        receiver.reset();
    }
    connected = false;
    markAllOffline();

    return 0;
}

void SparkplugHost::loop(bool &wasConnected)
{
    {
        std::lock_guard<std::mutex> guard(configLock);
        if (!receiver || reconfigure)
        {
            reconfigure = false;
            buildReceiver();
            wasConnected = false;
        }
    }

    bool isNowConnected = receiver->isConnected();
    if (wasConnected && !isNowConnected)
    {
        // Sparkplug: a host that loses its connection can no longer vouch for any edge node
        SPH_LOG_WARNING(COMPONENT, "Lost connection to the broker. Marking all nodes offline.");
        markAllOffline();
    }
    wasConnected = isNowConnected;
    connected = isNowConnected;

    std::set<std::string> rebirths;
    SparkplugMessage message;
    SparkplugTopic topic;
    size_t processed = 0;

    // Wait for the first message, then drain whatever else is queued
    auto wait = RECEIVE_WAIT;
    while (processed < MAX_MESSAGES_PER_ITERATION && receiver->consume(message, wait))
    {
        wait = std::chrono::milliseconds(0);
        processed++;

        if (!topic.parse(message.topic))
        {
            continue;
        }

        ParseResult result;
        {
            std::lock_guard<std::mutex> guard(payloadLock);
            result = process(topic, message.payload.get());
        }

        if (result == ParseResult::OUT_OF_SYNC)
        {
            rebirths.insert(nodeCommandTopic(topic.getGroup(), topic.getNode()));
        }
    }

    for (const auto &rebirthTopic : rebirths)
    {
        receiver->rebirth(rebirthTopic);
    }

    if (!receiver->isConnected())
    {
        return;
    }

    std::deque<SparkplugMessage> pending;
    {
        std::lock_guard<std::mutex> guard(commandLock);
        pending.swap(commands);
    }

    for (auto &command : pending)
    {
        if (!receiver->command(command))
        {
            SPH_LOG_WARNING(COMPONENT, "Dropped a command for {}", command.topic);
        }
    }
}

void SparkplugHost::stop()
{
    running = false;
}

ParseResult SparkplugHost::process(const SparkplugTopic &topic, const tahu::Payload *payload)
{
    return get(topic.getGroup())->process(topic, payload);
}

void SparkplugHost::buildReceiver()
{
    {
        std::lock_guard<std::mutex> guard(payloadLock);
        clear();
    }

    if (receiver)
    {
        receiver->stop();
        receiver.reset();
    }

    receiver = std::make_unique<SparkplugReceiver>(server, clientId, hostId);
    receiver->credentials(username, password);
    receiver->configure();
    receiver->activate();
}

void SparkplugHost::markAllOffline()
{
    std::lock_guard<std::mutex> guard(payloadLock);
    each([](Group *group)
         { group->markOffline(); });
}

std::vector<PublishableUpdate> SparkplugHost::getPayloads(bool force)
{
    std::vector<PublishableUpdate> payloads;
    std::lock_guard<std::mutex> guard(payloadLock);
    each([&payloads, force](Group *group)
         { group->appendTo(payloads, force); });
    return payloads;
}

void SparkplugHost::command(SparkplugMessage message)
{
    std::lock_guard<std::mutex> guard(commandLock);
    commands.push_back(std::move(message));
}

void SparkplugHost::commandNode(const std::string &group, const std::string &node, tahu::PayloadPtr payload)
{
    command(SparkplugMessage(nodeCommandTopic(group, node), std::move(payload)));
}

void SparkplugHost::commandDevice(const std::string &group, const std::string &node, const std::string &device, tahu::PayloadPtr payload)
{
    command(SparkplugMessage(SPARKPLUG_ID + "/" + group + "/DCMD/" + node + "/" + device, std::move(payload)));
}

void SparkplugHost::requestRebirth(const std::string &group, const std::string &node)
{
    tahu::PayloadPtr payload = tahu::makePayload();
    bool value = true;
    add_simple_metric(payload.get(), "Node Control/Rebirth", false, 0, METRIC_DATA_TYPE_BOOLEAN, false, false, &value, sizeof(value));
    commandNode(group, node, std::move(payload));
}

void SparkplugHost::configure(std::string address)
{
    std::lock_guard<std::mutex> guard(configLock);
    server = std::move(address);
    reconfigure = true;
}

void SparkplugHost::credentials(std::string username, std::string password)
{
    std::lock_guard<std::mutex> guard(configLock);
    this->username = std::move(username);
    this->password = std::move(password);
    reconfigure = true;
}

bool SparkplugHost::isConnected() const
{
    return connected;
}

void SparkplugHost::setRebirthRetryInterval(std::chrono::milliseconds interval)
{
    Node::setRebirthRetryInterval(interval);
}
