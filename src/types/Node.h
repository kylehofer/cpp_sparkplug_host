/*
 * File: Node.h
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

#ifndef SRC_TYPES_NODE
#define SRC_TYPES_NODE

#include "Publishable.h"
#include "Device.h"
#include "../DataCollection.h"
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

/**
 * @brief A Sparkplug Edge Node and its Devices. Its id is "group/node".
 *
 * Validates sequence numbers across all node and device messages, tracks the birth/death
 * sequence (bdSeq) so stale deaths are ignored, and throttles rebirth requests.
 */
class Node : public Publishable, private DataCollection<Device>
{
private:
    uint8_t sequence = 0;
    std::optional<uint64_t> bdSeq;
    bool rebirthRequested = false;
    std::chrono::steady_clock::time_point lastRebirthRequest;

    /**
     * @brief Decides whether a rebirth should be sent now.
     * Only one request is sent per retry interval until an NBIRTH arrives.
     *
     * @return ParseResult OUT_OF_SYNC if a rebirth should be sent, otherwise OK
     */
    ParseResult requestRebirth();

public:
    explicit Node(const std::string &name);

    /**
     * @brief Sets how long to wait for an NBIRTH before requesting another rebirth. Defaults to 5 seconds.
     *
     * @param interval
     */
    static void setRebirthRetryInterval(std::chrono::milliseconds interval);
    static std::chrono::milliseconds getRebirthRetryInterval();

    /**
     * @brief Processes a payload sent by this Node or one of its Devices.
     * Validates the sequence number and passes device messages to the Device.
     *
     * @param topic
     * @param payload
     * @return ParseResult OUT_OF_SYNC if a rebirth should be requested now
     */
    ParseResult process(const SparkplugTopic &topic, const tahu::Payload *payload);
    /**
     * @brief Appends the Node's update followed by its Devices' updates
     *
     * @param payloads
     * @param force
     */
    void appendTo(std::vector<PublishableUpdate> &payloads, bool force = false);
    /**
     * @brief Marks the Node and all of its Devices as offline
     *
     */
    void markOffline() override;
    /**
     * @brief Returns false as the Node is not a device
     *
     * @return false
     */
    bool isDevice() const override;
};

#endif /* SRC_TYPES_NODE */
