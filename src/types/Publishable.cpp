/*
 * File: Publishable.cpp
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

#include "Publishable.h"
#include "pb_decode.h"

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...)  \
    printf("Publishable: "); \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

using namespace std;

ParseResult Publishable::process(SparkplugTopic &topic, tahu::Payload *payload)
{
    if (topic.isDeath() && state == PublishableState::ACTIVE)
    {
        stale();
        changedState = ChangedState::CHANGES;
        return ParseResult::OK;
    }

    if (topic.isData())
    {
        if (state == PublishableState::STALE)
        {

            if (isDevice())
            {
                LOGGER("Received a Data message while stale for %s/%s/%s.\n",
                       topic.getGroup().c_str(),
                       topic.getNode().c_str(),
                       topic.getDevice().c_str());
            }
            else
            {
                LOGGER("Received a Data message while stale for %s/%s.\n",
                       topic.getGroup().c_str(),
                       topic.getNode().c_str());
            }

            if (actionState == ActionState::NOTHING)
            {
                actionState = ActionState::REBIRTH;
                return ParseResult::OUT_OF_SYNC;
            }
            else
            {
                return ParseResult::OK;
            }
        }
        lastValidMessage = payload->timestamp;
        loadPayload(payload);
        return ParseResult::OK;
    }

    if (topic.isBirth())
    {
        actionState = ActionState::NOTHING;
        changedState = ChangedState::CHANGES;
        birthed();
        lastValidMessage = payload->timestamp;
        loadPayload(payload, true);
        return ParseResult::OK;
    }

    if (topic.isCommand())
    {
        return ParseResult::OK;
    }

    return ParseResult::OK;
}

bool Publishable::hasMetric(tahu::Metric &input)
{
    return false;
}

void Publishable::stale()
{
    state = PublishableState::STALE;
}

void Publishable::birthed()
{
    state = PublishableState::BIRTHED;
}

void Publishable::active()
{
    state = PublishableState::ACTIVE;
}

Publishable::Publishable(std::string name) : name(name)
{
}

Publishable::~Publishable()
{
}

void Publishable::appendTo(std::vector<PublishableUpdate> &payloads, bool force)
{
    if ((changedState == ChangedState::CHANGES || force) && state == PublishableState::STALE)
    {
        if (!force)
        {
            changedState = ChangedState::NOTHING;
        }
        payloads.push_back(PublishableUpdate(nullptr, name, UpdateType::DEATH));
        return;
    }

    changedState = ChangedState::NOTHING;

    bool hasChanges = force || any([](Metric *metric)
                                   { return metric->isDirty(); });

    if (!hasChanges)
    {
        return;
    }

    tahu::Payload *payload = (org_eclipse_tahu_protobuf_Payload *)malloc(sizeof(org_eclipse_tahu_protobuf_Payload));

    // Initialize payload
    memset(payload, 0, sizeof(org_eclipse_tahu_protobuf_Payload));

    payload->has_seq = false;
    payload->has_timestamp = true;
    payload->timestamp = lastValidMessage;

    each([payload, force](Metric *metric)
         { metric->appendTo(payload, force); });

    if (state == PublishableState::BIRTHED || force)
    {
        if (state == PublishableState::BIRTHED)
        {
            active();
        }
        payloads.push_back(PublishableUpdate(payload, name, UpdateType::BIRTH));
    }
    else
    {
        payloads.push_back(PublishableUpdate(payload, name, UpdateType::PUBLISH));
    }
}

ParseResult Publishable::loadPayload(tahu::Payload *payload, bool isBirth)
{
    if (isBirth)
    {
        clear();
    }

    for (size_t i = 0; i < payload->metrics_count; i++)
    {
        tahu::Metric *metric = &payload->metrics[i];
        if (get(metric->name)->process(metric) == ParseResult::OUT_OF_SYNC)
        {
            return ParseResult::OUT_OF_SYNC;
        };
    }
    return ParseResult::OK;
}
