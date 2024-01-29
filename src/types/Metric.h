/*
 * File: Metric.h
 * Project: cpp_sparkplug_host
 * Created Date: Monday December 25th 2023
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

#ifndef SRC_TYPES_METRIC
#define SRC_TYPES_METRIC

#include "TahuTypes.h"
#include "PropertySet.h"
#include "CommonTypes.h"
#include <string>

union MetricFlags
{
    uint8_t data;
    struct
    {
        uint8_t hasTimestamp : 1;
        uint8_t isHistorical : 1;
        uint8_t isTransient : 1;
        uint8_t isNull : 1;
        uint8_t hasMetadata : 1;
        uint8_t hasProperties : 1;
        uint8_t dirty : 1;
        uint8_t reserved : 1;
    };
};

/**
 * @brief A class that represents a Sparkplug Metric
 *
 */
class Metric
{
private:
    tahu::Metric *source;
    PropertySet propertySet;
    void *data = nullptr;
    size_t length = 0;
    pb_size_t whichValue;
    uint32_t type;
    std::string name;
    uint64_t timestamp;
    MetricFlags flags;

protected:
    void clear();

public:
    Metric(std::string &name);
    ~Metric();
    /**
     * @brief Appends this Metric to a Payload if it has had changes
     *
     * @param payload
     * @param force Forces the Metric to be appended
     */
    void appendTo(tahu::Payload *payload, bool force = false);
    /**
     * @brief Processes a tahu::Metric and updates the Metric
     *
     * @param metric
     * @return ParseResult
     */
    ParseResult process(tahu::Metric *metric);
    /**
     * @brief Whether the metric has data that hasn't been acknowledged
     *
     * @return true
     * @return false
     */
    bool isDirty();
};

#endif /* SRC_TYPES_METRIC */
