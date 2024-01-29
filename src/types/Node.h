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
#include <map>
#include <string>

class Node : public Publishable, DataCollection<Device>
{

private:
    uint8_t sequence = 0;

protected:
public:
    Node(){};
    Node(std::string name) : Publishable(name){};
    /**
     * @brief Processes a Payload that has come from a topic
     * Will validate the sequence and sparkplug payload.
     * If the payload is for a device of the Node, it will be passed through.
     *
     * @param topic
     * @param payload
     * @return ParseResult
     */
    ParseResult process(SparkplugTopic &topic, tahu::Payload *payload);
    /**
     * @brief Appends the Node's and Device's payloads if it they changes
     *
     * @param payloads
     * @param force
     */
    void appendTo(std::vector<PublishableUpdate> &payloads, bool force = false);
    /**
     * @brief Returns false as the Node is not a device
     *
     * @return true
     * @return false
     */
    virtual bool isDevice() override;
    bool isDirty();
};

#endif /* SRC_TYPES_NODE */
