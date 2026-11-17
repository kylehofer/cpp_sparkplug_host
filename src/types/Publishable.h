/*
 * File: Publishable.h
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

#ifndef SRC_TYPES_PUBLISHABLE
#define SRC_TYPES_PUBLISHABLE

#include "tahu.h"
#include "../DataCollection.h"
#include "../utilities/SparkplugTopic.h"
#include "CommonTypes.h"
#include "Metric.h"
#include "PublishableUpdate.h"
#include "TahuTypes.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Whether a Node or Device is currently online
 *
 */
enum class PublishableState
{
    ONLINE,
    OFFLINE
};

/**
 * @brief Common state for a Sparkplug Node or Device: online state, metrics and metric aliases.
 *
 */
class Publishable : public DataCollection<Metric>
{
private:
    std::map<uint64_t, std::string> aliases;
    uint64_t lastTimestamp = 0;
    bool birthPending = false;
    bool deathPending = false;

protected:
    std::string name;
    PublishableState state = PublishableState::OFFLINE;

    /**
     * @brief Loads the metrics of a payload. Every metric is processed, even if an earlier one fails.
     *
     * @param payload
     * @param isBirth Births replace all metrics and define aliases. Data may only update known metrics.
     * @return ParseResult OUT_OF_SYNC if any metric was unknown, had an unknown alias, or changed type
     */
    ParseResult loadPayload(const tahu::Payload *payload, bool isBirth);

public:
    explicit Publishable(const std::string &name);
    virtual ~Publishable();

    /**
     * @brief Appends an update describing what changed since the last call.
     *
     * Appends a BIRTH after the entity was birthed, a DEATH after it went offline,
     * or a PUBLISH holding the changed metrics. Appends nothing if nothing changed.
     *
     * @param payloads
     * @param force Append a full BIRTH (or DEATH if offline) without consuming any pending changes
     */
    void appendTo(std::vector<PublishableUpdate> &payloads, bool force = false);
    /**
     * @brief Processes a birth, death or data message for this entity.
     * Sequence numbers are handled by the owning Node.
     *
     * @param topic
     * @param payload
     * @return ParseResult OUT_OF_SYNC if a rebirth is needed
     */
    ParseResult process(const SparkplugTopic &topic, const tahu::Payload *payload);

    /**
     * @brief Marks the entity as offline. A DEATH update is reported on the next appendTo().
     *
     */
    virtual void markOffline();

    /**
     * @brief Whether the entity is currently online
     *
     * @return true
     * @return false
     */
    bool isOnline() const;

    /**
     * @brief The entity's id: "group/node" or "group/node/device"
     *
     * @return const std::string&
     */
    const std::string &getName() const;

    /**
     * @brief Whether the Publisher is a Device
     *
     * @return true
     * @return false
     */
    virtual bool isDevice() const = 0;
};

#endif /* SRC_TYPES_PUBLISHABLE */
