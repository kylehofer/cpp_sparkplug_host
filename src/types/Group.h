/*
 * File: Group.h
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

#ifndef SRC_TYPES_GROUP
#define SRC_TYPES_GROUP

#include <string>
#include <vector>
#include "Node.h"
#include "PublishableUpdate.h"
#include "../DataCollection.h"

/**
 * @brief All Nodes seen on a Sparkplug Group
 *
 */
class Group : public DataCollection<Node>
{
private:
    std::string name;

public:
    explicit Group(const std::string &name) : name(name) {}
    /**
     * @brief Processes a Payload for a Node or Device on this group
     *
     * @param topic
     * @param payload
     * @return ParseResult OUT_OF_SYNC if a rebirth should be requested for the node
     */
    ParseResult process(const SparkplugTopic &topic, const tahu::Payload *payload);
    /**
     * @brief Appends updates from every Node and Device on this Group
     *
     * @param payloads
     * @param force Return a full snapshot rather than changes
     */
    void appendTo(std::vector<PublishableUpdate> &payloads, bool force = false);
    /**
     * @brief Marks every Node and Device on this Group as offline
     *
     */
    void markOffline();
};

#endif /* SRC_TYPES_GROUP */
