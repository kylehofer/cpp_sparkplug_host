/*
 * File: PublishableUpdate.h
 * Project: cpp_sparkplug_host
 * Created Date: Monday January 22nd 2024
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

#ifndef SRC_TYPES_PUBLISHABLEUPDATE
#define SRC_TYPES_PUBLISHABLEUPDATE

#include "TahuTypes.h"
#include <string>

/**
 * @brief The kind of change a PublishableUpdate describes.
 *
 */
enum class UpdateType
{
    /** Some metrics changed. The payload holds only the changed metrics. */
    PUBLISH = 0,
    /** The node or device went offline. There is no payload. */
    DEATH,
    /** The node or device came online, or a full snapshot was requested. The payload holds every metric. */
    BIRTH
};

/**
 * @brief A change to a Sparkplug Node or Device, as returned by SparkplugHost::getPayloads().
 *
 * The update owns its payload, which is released when the update is destroyed.
 * Updates are move-only. Do not call free_payload() or free() on the payload yourself.
 */
class PublishableUpdate
{
public:
    /** Metrics for BIRTH and PUBLISH updates. Null for DEATH updates. */
    tahu::PayloadPtr payload;
    /** "group/node" for nodes, "group/node/device" for devices. */
    std::string id;
    UpdateType type = UpdateType::DEATH;

    PublishableUpdate() = default;
    PublishableUpdate(tahu::PayloadPtr payload, std::string id, UpdateType type);
};

#endif /* SRC_TYPES_PUBLISHABLEUPDATE */
