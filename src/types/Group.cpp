/*
 * File: Group.cpp
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

#include "Group.h"
#include <functional> //for std::hash

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...) \
    printf("Group: ");      \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

ParseResult Group::process(SparkplugTopic &topic, tahu::Payload *payload)
{
    auto nodeSource = std::string(topic.getGroup() + "/" + topic.getNode());
    auto node = get(nodeSource);
    return node->process(topic, payload);
}

void Group::appendTo(std::vector<PublishableUpdate> &payloads, bool force)
{
    each([&payloads, force](Node *node)
         { node->appendTo(payloads, force); });
}
