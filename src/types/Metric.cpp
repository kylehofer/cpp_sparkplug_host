/*
 * File: Metric.cpp
 * Project: cpp_sparkplug_host
 * Created Date: Monday December 25th 2023
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

#include "Metric.h"

using namespace std;

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...) \
    printf("Metric: ");     \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

void Metric::appendTo(tahu::Payload *payload, bool force)
{
    if (!flags.dirty && !force)
    {
        return;
    }

    if (!force)
    {
        flags.dirty = false;
    }

    tahu::Metric *metric;

    metric = (tahu::Metric *)malloc(sizeof(tahu::Metric));
    memset(metric, 0, sizeof(tahu::Metric));

    propertySet.appendTo(metric, force);

    metric->has_alias = false;
    metric->has_is_historical = metric->is_historical = flags.isHistorical;
    metric->has_is_transient = metric->is_transient = flags.isTransient;
    metric->has_is_null = metric->is_null = flags.isNull;
    metric->has_timestamp = flags.hasTimestamp;
    metric->datatype = this->type;
    metric->has_datatype = true;
    metric->which_value = this->whichValue;

    metric->timestamp = timestamp;

    metric->name = strdup(name.c_str());

    memcpy(&metric->value, &value, sizeof(value));

    add_metric_to_payload(payload, metric);

    free(metric);
}

inline void Metric::clearValue()
{
    switch (type)
    {
    case METRIC_DATA_TYPE_STRING:
    case METRIC_DATA_TYPE_TEXT:
        free(value.stringValue);
        break;
    default:
        break;
    }
    memset(&value, 0, sizeof(value));
}

ParseResult Metric::process(tahu::Metric *metric)
{
    flags.dirty = true;

    if (metric->datatype != type && type != PROPERTY_DATA_TYPE_UNKNOWN)
    {
        return ParseResult::OUT_OF_SYNC;
    }

    if (metric->has_properties)
    {
        propertySet.process(&metric->properties);
    }

    flags.isHistorical = metric->has_is_historical && metric->is_historical;
    flags.isTransient = metric->has_is_transient && metric->is_transient;
    flags.hasTimestamp = metric->has_timestamp;
    flags.hasMetadata = metric->has_metadata;

    if (flags.hasTimestamp)
    {
        timestamp = metric->timestamp;
    }
    else
    {
        timestamp = 0;
    }

    if ((metric->has_is_null && !metric->is_null) || !metric->has_is_null)
    {
        flags.isNull = false;

        switch (metric->datatype)
        {
        case METRIC_DATA_TYPE_INT8:
        case METRIC_DATA_TYPE_UINT8:
            length = sizeof(int8_t);
            value.intValue = metric->value.int_value;
            break;
        case METRIC_DATA_TYPE_INT16:
        case METRIC_DATA_TYPE_UINT16:
            length = sizeof(int16_t);
            value.intValue = metric->value.int_value;
            break;
        case METRIC_DATA_TYPE_INT32:
        case METRIC_DATA_TYPE_UINT32:
            length = sizeof(int32_t);
            value.intValue = metric->value.int_value;
            break;
        case METRIC_DATA_TYPE_INT64:
        case METRIC_DATA_TYPE_UINT64:
        case METRIC_DATA_TYPE_DATETIME:
            length = sizeof(int64_t);
            value.longValue = metric->value.long_value;
            break;
        case METRIC_DATA_TYPE_FLOAT:
            length = sizeof(float);
            value.floatValue = metric->value.float_value;
            break;
        case METRIC_DATA_TYPE_DOUBLE:
            length = sizeof(double);
            value.doubleValue = metric->value.double_value;
            break;
        case METRIC_DATA_TYPE_BOOLEAN:
            length = sizeof(bool);
            value.booleanValue = metric->value.boolean_value;
            break;
        case METRIC_DATA_TYPE_STRING:
        case METRIC_DATA_TYPE_TEXT:
            if (value.stringValue)
            {
                free(value.stringValue);
            }
            length = strlen(metric->value.string_value);
            value.stringValue = strdup(metric->value.string_value);
            break;
        case METRIC_DATA_TYPE_BYTES:
        case METRIC_DATA_TYPE_DATASET:
        case METRIC_DATA_TYPE_UUID:
        case METRIC_DATA_TYPE_UNKNOWN:
        default:
            flags.isNull = true;
            clearValue();
            break;
        }
    }
    else
    {
        clearValue();
    }

    whichValue = metric->which_value;
    type = metric->datatype;

    return ParseResult::OK;
}

bool Metric::isDirty()
{
    return flags.dirty;
}

void Metric::clear()
{
    clearValue();

    flags.data = 0;
    timestamp = 0;
    type = METRIC_DATA_TYPE_UNKNOWN;
}

Metric::Metric(string &name)
{
    clearValue();
    this->name = string(name);
}

Metric::~Metric()
{
    clear();
}
