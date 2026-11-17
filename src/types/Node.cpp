/*
 * File: Node.cpp
 * Project: cpp_sparkplug_host
 * Created Date: Thursday December 21st 2023
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

#include "Node.h"
#include "../utilities/LogMacros.h"
#include <atomic>
#include <cstring>

namespace
{
    const char *const COMPONENT = "Node";
    const char *const BDSEQ_METRIC = "bdSeq";

    std::atomic<int64_t> rebirthRetryIntervalMs{5000};

    std::optional<uint64_t> findBdSeq(const tahu::Payload *payload)
    {
        for (size_t i = 0; i < payload->metrics_count; i++)
        {
            const tahu::Metric *metric = &payload->metrics[i];
            if (metric->name && strcmp(metric->name, BDSEQ_METRIC) == 0)
            {
                return tahu::readUnsigned(metric);
            }
        }
        return std::nullopt;
    }
}

Node::Node(const std::string &name) : Publishable(name)
{
}

void Node::setRebirthRetryInterval(std::chrono::milliseconds interval)
{
    rebirthRetryIntervalMs = interval.count();
}

std::chrono::milliseconds Node::getRebirthRetryInterval()
{
    return std::chrono::milliseconds(rebirthRetryIntervalMs.load());
}

ParseResult Node::requestRebirth()
{
    auto now = std::chrono::steady_clock::now();
    if (rebirthRequested && now - lastRebirthRequest < getRebirthRetryInterval())
    {
        return ParseResult::OK;
    }
    rebirthRequested = true;
    lastRebirthRequest = now;
    return ParseResult::OUT_OF_SYNC;
}

ParseResult Node::process(const SparkplugTopic &topic, const tahu::Payload *payload)
{
    if (topic.isCommand())
    {
        return ParseResult::OK;
    }

    switch (topic.getCommandType())
    {
    case SparkplugCommandType::NBIRTH:
    {
        sequence = payload->has_seq ? static_cast<uint8_t>(payload->seq + 1) : 1;
        bdSeq = findBdSeq(payload);

        // Devices must send a new DBIRTH after their node rebirths
        DataCollection<Device>::each([](Device *device)
                                     { device->markOffline(); });

        if (Publishable::process(topic, payload) == ParseResult::OUT_OF_SYNC)
        {
            // Keep the throttle running so a node with a bad birth is not asked to rebirth on every birth
            return requestRebirth();
        }
        rebirthRequested = false;
        return ParseResult::OK;
    }
    case SparkplugCommandType::NDEATH:
    {
        auto deathBdSeq = findBdSeq(payload);
        if (bdSeq && deathBdSeq && *bdSeq != *deathBdSeq)
        {
            SPH_LOG_INFO(COMPONENT, "Ignoring NDEATH for {} from an earlier session (bdSeq {}, current {})",
                         name,
                         *deathBdSeq,
                         *bdSeq);
            return ParseResult::OK;
        }
        markOffline();
        return ParseResult::OK;
    }
    default:
        break;
    }

    // NDATA, DBIRTH, DDATA and DDEATH all require an online node with a known sequence
    if (!isOnline())
    {
        SPH_LOG_INFO(COMPONENT, "Received a message for {} while it is offline", name);
        return requestRebirth();
    }

    bool outOfSync = false;

    if (payload->has_seq)
    {
        if (payload->seq != sequence)
        {
            SPH_LOG_WARNING(COMPONENT, "Sequence mismatch for {}. Expected {}. Received: {}",
                            name, sequence, payload->seq);
            outOfSync = true;
        }
        // Resynchronise so one lost message does not flag every message after it
        sequence = static_cast<uint8_t>(payload->seq + 1);
    }

    ParseResult result = ParseResult::OK;

    if (!topic.isDevice())
    {
        result = Publishable::process(topic, payload);
    }
    else
    {
        std::string deviceName = name + "/" + topic.getDevice();
        Device *device = topic.isBirth()
                             ? DataCollection<Device>::get(deviceName)
                             : DataCollection<Device>::find(deviceName);

        if (device)
        {
            result = device->process(topic, payload);
        }
        else if (topic.isData())
        {
            SPH_LOG_INFO(COMPONENT, "Received data for unknown device {}", deviceName);
            result = ParseResult::OUT_OF_SYNC;
        }
    }

    if (outOfSync || result == ParseResult::OUT_OF_SYNC)
    {
        return requestRebirth();
    }

    return ParseResult::OK;
}

void Node::appendTo(std::vector<PublishableUpdate> &payloads, bool force)
{
    Publishable::appendTo(payloads, force);
    DataCollection<Device>::each([&payloads, force](Device *device)
                                 { device->appendTo(payloads, force); });
}

void Node::markOffline()
{
    Publishable::markOffline();
    rebirthRequested = false;
    DataCollection<Device>::each([](Device *device)
                                 { device->markOffline(); });
}

bool Node::isDevice() const
{
    return false;
}
