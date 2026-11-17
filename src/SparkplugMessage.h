/*
 * File: SparkplugMessage.h
 * Project: cpp_sparkplug_host
 * Created Date: Friday November 6th 2026
 * Author: Kyle Hofer
 *
 * MIT License
 *
 * Copyright (c) 2026 Kyle Hofer
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

#ifndef SRC_SPARKPLUGMESSAGE
#define SRC_SPARKPLUGMESSAGE

#include "types/TahuTypes.h"
#include <string>
#include <utility>

/**
 * @brief A Sparkplug payload together with the topic it was, or will be, published on.
 *
 * The message owns its payload. Constructing a message from a raw pointer takes ownership
 * of it, so the pointer must come from malloc/calloc (or tahu::makePayload().release())
 * and must not be freed by the caller afterwards.
 */
struct SparkplugMessage
{
    SparkplugMessage() = default;
    SparkplugMessage(std::string topic, tahu::PayloadPtr payload)
        : topic(std::move(topic)), payload(std::move(payload)) {}
    SparkplugMessage(std::string topic, tahu::Payload *payload)
        : topic(std::move(topic)), payload(payload) {}

    std::string topic;
    tahu::PayloadPtr payload;
};

#endif /* SRC_SPARKPLUGMESSAGE */
