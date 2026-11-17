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

/**
 * @brief The last known value of a Sparkplug Metric.
 *
 * Supports the scalar datatypes, String and Text. DataSet, Bytes, File, UUID and Template
 * metrics are tracked by name and type but their values are reported as null.
 */
class Metric
{
private:
    std::string name;
    PropertySet propertySet;
    uint32_t type = METRIC_DATA_TYPE_UNKNOWN;
    uint64_t timestamp = 0;
    bool hasTimestamp = false;
    bool isHistorical = false;
    bool isTransient = false;
    bool isNull = true;
    bool dirty = false;
    pb_size_t integerTag = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;

    union MetricValueUnion
    {
        uint64_t longValue;
        float floatValue;
        double doubleValue;
        bool booleanValue;
    } value{};

    std::string stringValue;

    /**
     * @brief Whether the value of this metric can be stored and republished
     */
    bool isSupportedType() const;

public:
    explicit Metric(const std::string &name);
    Metric(const Metric &) = delete;
    Metric &operator=(const Metric &) = delete;

    /**
     * @brief Appends this Metric to a Payload if it has had changes
     *
     * @param payload
     * @param force Append the metric even if it has not changed
     * @param consume Mark the metric as clean once appended
     */
    void appendTo(tahu::Payload *payload, bool force = false, bool consume = true);
    /**
     * @brief Updates the Metric from a received tahu::Metric.
     * The datatype may be omitted on data messages, in which case the type from the birth is used.
     *
     * @param metric
     * @return ParseResult OUT_OF_SYNC if the datatype is unknown or differs from the birth
     */
    ParseResult process(const tahu::Metric *metric);
    /**
     * @brief Whether the metric has changed since it was last appended
     *
     * @return true
     * @return false
     */
    bool isDirty() const;
    /**
     * @brief The Sparkplug datatype of the metric (METRIC_DATA_TYPE_*)
     */
    uint32_t getType() const;
};

#endif /* SRC_TYPES_METRIC */
