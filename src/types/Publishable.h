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
#include <vector>
#include <string>
#include "../SparkplugReceiver.h"
#include "../DataCollection.h"
#include "../utilities/SparkplugTopic.h"
#include "PublishableUpdate.h"
#include "CommonTypes.h"
#include "TahuTypes.h"
#include "Metric.h"
#include <time.h>

enum class PublishableState
{
    ACTIVE,
    BIRTHED,
    STALE
};

enum class ActionState
{
    NOTHING,
    REBIRTH
};

enum class ChangedState
{
    NOTHING,
    CHANGES
};

/**
 * @brief A class for representing a Sparkplug Publishable (e.g. Node, Device).
 *
 */
class Publishable : public DataCollection<Metric>
{
private:
    /**
     * @brief Loads the payload into the Publishable
     *
     * @param payload
     * @param isBirth
     * @return ParseResult
     */
    ParseResult loadPayload(tahu::Payload *payload, bool isBirth = false);
    time_t lastValidMessage = 0;

protected:
    std::string name;
    PublishableState state = PublishableState::STALE;
    ActionState actionState = ActionState::NOTHING;
    ChangedState changedState = ChangedState::NOTHING;

public:
    /**
     * @brief Construct a new Publishable object
     *
     */
    Publishable(){};
    /**
     * @brief Construct a new Publishable with a unique name
     *
     * @param name
     */
    Publishable(std::string name);
    virtual ~Publishable();
    /**
     * @brief Creates and appends a Payload to the list containing
     * all the data of this Publisher
     *
     * @param payloads
     * @param force
     */
    void appendTo(std::vector<PublishableUpdate> &payloads, bool force = false);
    /**
     * @brief Processes a payload for this Publisher
     * Will load all data from Metrics
     *
     * @param topic
     * @param payload
     * @return ParseResult
     */
    ParseResult process(SparkplugTopic &topic, tahu::Payload *payload);

    bool hasMetric(tahu::Metric &metric);
    /**
     * @brief Marks the Publisher as Stale (Dead)
     *
     */
    void stale();
    /**
     * @brief Marks the Publisher as Birthed (Alive)
     *
     */
    void birthed();
    /**
     * @brief Marks the Publisher as Active (Alive)
     *
     */
    void active();
    /**
     * @brief Whether the Publisher is a Device
     *
     * @return true
     * @return false
     */
    virtual inline bool isDevice() = 0;
};

#endif /* SRC_TYPES_PUBLISHABLE */
