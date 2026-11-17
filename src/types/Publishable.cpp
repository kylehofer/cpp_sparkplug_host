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
#include "../utilities/LogMacros.h"

namespace
{
    const char *const COMPONENT = "Publishable";
}

Publishable::Publishable(const std::string &name) : name(name)
{
}

Publishable::~Publishable() = default;

ParseResult Publishable::process(const SparkplugTopic &topic, const tahu::Payload *payload)
{
    if (topic.isBirth())
    {
        clear();
        aliases.clear();
        state = PublishableState::ONLINE;
        birthPending = true;
        deathPending = false;
        if (payload->has_timestamp)
        {
            lastTimestamp = payload->timestamp;
        }
        return loadPayload(payload, true);
    }

    if (topic.isDeath())
    {
        markOffline();
        return ParseResult::OK;
    }

    if (topic.isData())
    {
        if (state != PublishableState::ONLINE)
        {
            SPH_LOG_INFO(COMPONENT, "Received data for {} while it is offline", name);
            return ParseResult::OUT_OF_SYNC;
        }
        if (payload->has_timestamp)
        {
            lastTimestamp = payload->timestamp;
        }
        return loadPayload(payload, false);
    }

    return ParseResult::OK;
}

void Publishable::markOffline()
{
    if (state == PublishableState::ONLINE)
    {
        state = PublishableState::OFFLINE;
        deathPending = true;
        birthPending = false;
    }
}

bool Publishable::isOnline() const
{
    return state == PublishableState::ONLINE;
}

const std::string &Publishable::getName() const
{
    return name;
}

void Publishable::appendTo(std::vector<PublishableUpdate> &payloads, bool force)
{
    if (state != PublishableState::ONLINE)
    {
        if (deathPending || force)
        {
            payloads.emplace_back(nullptr, name, UpdateType::DEATH);
            if (!force)
            {
                deathPending = false;
            }
        }
        return;
    }

    bool isBirth = birthPending || force;

    if (!isBirth && !any([](Metric *metric)
                         { return metric->isDirty(); }))
    {
        return;
    }

    tahu::PayloadPtr payload = tahu::makePayload();
    payload->has_seq = false;
    payload->has_timestamp = true;
    payload->timestamp = lastTimestamp;

    // A forced snapshot must not consume changes the incremental stream has not reported yet
    bool consume = !force;
    each([&payload, isBirth, consume](Metric *metric)
         { metric->appendTo(payload.get(), isBirth, consume); });

    if (isBirth)
    {
        if (!force)
        {
            birthPending = false;
        }
        payloads.emplace_back(std::move(payload), name, UpdateType::BIRTH);
    }
    else
    {
        payloads.emplace_back(std::move(payload), name, UpdateType::PUBLISH);
    }
}

ParseResult Publishable::loadPayload(const tahu::Payload *payload, bool isBirth)
{
    ParseResult result = ParseResult::OK;

    for (size_t i = 0; i < payload->metrics_count; i++)
    {
        const tahu::Metric *source = &payload->metrics[i];
        std::string metricName;

        if (source->name && source->name[0] != '\0')
        {
            metricName = source->name;
            if (isBirth && source->has_alias)
            {
                aliases[source->alias] = metricName;
            }
        }
        else if (source->has_alias && !isBirth)
        {
            auto alias = aliases.find(source->alias);
            if (alias == aliases.end())
            {
                SPH_LOG_WARNING(COMPONENT, "Unknown alias {} on {}",
                                source->alias, name);
                result = ParseResult::OUT_OF_SYNC;
                continue;
            }
            metricName = alias->second;
        }
        else
        {
            SPH_LOG_WARNING(COMPONENT, "Metric without a name or alias on {}", name);
            result = ParseResult::OUT_OF_SYNC;
            continue;
        }

        Metric *metric = isBirth ? get(metricName) : find(metricName);
        if (!metric)
        {
            SPH_LOG_WARNING(COMPONENT, "Metric {} on {} was not in its birth", metricName, name);
            result = ParseResult::OUT_OF_SYNC;
            continue;
        }

        if (metric->process(source) == ParseResult::OUT_OF_SYNC)
        {
            SPH_LOG_WARNING(COMPONENT, "Metric {} on {} has an unexpected datatype", metricName, name);
            result = ParseResult::OUT_OF_SYNC;
        }
    }

    return result;
}
