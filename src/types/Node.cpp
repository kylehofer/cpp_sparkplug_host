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

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...) \
    printf("Node: ");       \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

ParseResult Node::process(SparkplugTopic &topic, tahu::Payload *payload)
{
    if (topic.isBirth() && !topic.isDevice())
    {
        sequence = 0;
    }

    if (payload->has_seq && sequence != payload->seq)
    {
        LOGGER("Sequence mismatch for %s/%s. Expected %u. Received: %lu.\n",
               topic.getGroup().c_str(),
               topic.getNode().c_str(),
               sequence, payload->seq);
        return ParseResult::OUT_OF_SYNC;
    }

    sequence += 1;

    if (!topic.isDevice())
    {
        ParseResult result = Publishable::process(topic, payload);
        if (result == ParseResult::OUT_OF_SYNC)
        {
            // Rebirth;
        }
        return result;
    }
    else
    {
        auto deviceSource = std::string(name + "/" + topic.getDevice());
        auto device = DataCollection<Device>::get(deviceSource);
        ParseResult result = device->process(topic, payload);
        if (result == ParseResult::OUT_OF_SYNC)
        {
            // Rebirth;
        }
        return result;
    }
}

void Node::appendTo(std::vector<PublishableUpdate> &payloads, bool force)
{
    Publishable::appendTo(payloads, force);
    DataCollection<Device>::each([&payloads, force](Device *device)
                                 { device->appendTo(payloads, force); });
}

inline bool Node::isDevice()
{
    return false;
}
