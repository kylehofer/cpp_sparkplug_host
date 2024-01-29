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
#include "utilities/SparkplugTopic.h"
#include "types/PublishableUpdate.h"
#include <thread>
#include <chrono>

const string delimiter{"/"};
const string SPARKPLUG_ID{"spBv1.0"};

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...)    \
    printf("SparkplugHost: "); \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

int SparkplugHost::run()
{
    SparkplugReceiver receiver(server, clientId);

    running = true;

    receiver.configure();
    receiver.activate();
    while (running)
    {
        SparkplugMessage message;
        SparkplugTopic topic;
        map<string, int> rebirths;

        while (receiver.consume(message))
        {
            if (topic.parse(message.topic))
            {
                ParseResult result;
                {
                    lock_guard<mutex> guard(stateLock);
                    result = process(topic, message.payload);
                }
                if (result == ParseResult::OUT_OF_SYNC)
                {
                    LOGGER("Receieved a message out of sync\n");
                    string value(SPARKPLUG_ID + "/" + topic.getGroup() + "/NCMD/" + topic.getNode());
                    rebirths[value] = 1;
                }
                free_payload(message.payload);
                free(message.payload);
            }
        }

        for (auto item = rebirths.begin(); item != rebirths.end(); ++item)
        {
            receiver.rebirth(item->first);
        }

        {
            lock_guard<mutex> guard(commandLock);

            while (commands.size() > 0)
            {
                auto command = commands.back();
                commands.pop_back();

                receiver.command(command);

                free_payload(command.payload);
                free(command.payload);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}

void SparkplugHost::stop()
{
    running = false;
}

ParseResult SparkplugHost::process(SparkplugTopic &topic, tahu::Payload *payload)
{
    auto group = get(topic.getGroup());
    return group->process(topic, payload);
}

vector<PublishableUpdate> SparkplugHost::getPayloads(bool force)
{
    // map<std::string, tahu::Payload *> payloads;
    vector<PublishableUpdate> payloads;

    {
        lock_guard<mutex> guard(stateLock);
        each([&payloads, force](Group *group)
             { group->appendTo(payloads, force); });
    }

    return payloads;
}

void SparkplugHost::command(SparkplugMessage message)
{
    lock_guard<mutex> guard(commandLock);
    commands.push_back(message);
}

SparkplugHost::SparkplugHost(std::string server, std::string clientId) : server(server), clientId(clientId)
{
}