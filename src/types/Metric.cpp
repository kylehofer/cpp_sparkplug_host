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
#include <cstring>

Metric::Metric(const std::string &name) : name(name)
{
}

bool Metric::isSupportedType() const
{
    switch (type)
    {
    case METRIC_DATA_TYPE_INT8:
    case METRIC_DATA_TYPE_UINT8:
    case METRIC_DATA_TYPE_INT16:
    case METRIC_DATA_TYPE_UINT16:
    case METRIC_DATA_TYPE_INT32:
    case METRIC_DATA_TYPE_UINT32:
    case METRIC_DATA_TYPE_INT64:
    case METRIC_DATA_TYPE_UINT64:
    case METRIC_DATA_TYPE_DATETIME:
    case METRIC_DATA_TYPE_FLOAT:
    case METRIC_DATA_TYPE_DOUBLE:
    case METRIC_DATA_TYPE_BOOLEAN:
    case METRIC_DATA_TYPE_STRING:
    case METRIC_DATA_TYPE_TEXT:
        return true;
    default:
        return false;
    }
}

void Metric::appendTo(tahu::Payload *payload, bool force, bool consume)
{
    if (!dirty && !force)
    {
        return;
    }

    if (consume)
    {
        dirty = false;
    }

    tahu::Metric metric;
    memset(&metric, 0, sizeof(tahu::Metric));

    propertySet.appendTo(&metric, force, consume);

    metric.name = strdup(name.c_str());
    metric.has_datatype = true;
    metric.datatype = type;
    metric.has_timestamp = hasTimestamp;
    metric.timestamp = timestamp;
    metric.has_is_historical = isHistorical;
    metric.is_historical = isHistorical;
    metric.has_is_transient = isTransient;
    metric.is_transient = isTransient;

    if (isNull || !isSupportedType())
    {
        metric.has_is_null = true;
        metric.is_null = true;
    }
    else
    {
        switch (type)
        {
        case METRIC_DATA_TYPE_INT8:
        case METRIC_DATA_TYPE_UINT8:
        case METRIC_DATA_TYPE_INT16:
        case METRIC_DATA_TYPE_UINT16:
        case METRIC_DATA_TYPE_INT32:
        case METRIC_DATA_TYPE_UINT32:
            // Republish small integers in the same field they were received in
            if (integerTag == org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag)
            {
                metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
                metric.value.long_value = value.longValue;
            }
            else
            {
                metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
                metric.value.int_value = static_cast<uint32_t>(value.longValue);
            }
            break;
        case METRIC_DATA_TYPE_INT64:
        case METRIC_DATA_TYPE_UINT64:
        case METRIC_DATA_TYPE_DATETIME:
            metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
            metric.value.long_value = value.longValue;
            break;
        case METRIC_DATA_TYPE_FLOAT:
            metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag;
            metric.value.float_value = value.floatValue;
            break;
        case METRIC_DATA_TYPE_DOUBLE:
            metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag;
            metric.value.double_value = value.doubleValue;
            break;
        case METRIC_DATA_TYPE_BOOLEAN:
            metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag;
            metric.value.boolean_value = value.booleanValue;
            break;
        case METRIC_DATA_TYPE_STRING:
        case METRIC_DATA_TYPE_TEXT:
            metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag;
            metric.value.string_value = strdup(stringValue.c_str());
            break;
        default:
            break;
        }
    }

    // add_metric_to_payload takes a shallow copy; the payload now owns the name, value and properties
    add_metric_to_payload(payload, &metric);
}

ParseResult Metric::process(const tahu::Metric *metric)
{
    uint32_t incomingType = (metric->has_datatype && metric->datatype != METRIC_DATA_TYPE_UNKNOWN)
                                ? metric->datatype
                                : type;

    if (incomingType == METRIC_DATA_TYPE_UNKNOWN)
    {
        // No datatype in this message and none known from a birth
        return ParseResult::OUT_OF_SYNC;
    }

    if (type != METRIC_DATA_TYPE_UNKNOWN && incomingType != type)
    {
        return ParseResult::OUT_OF_SYNC;
    }

    type = incomingType;
    dirty = true;

    ParseResult result = ParseResult::OK;
    if (metric->has_properties)
    {
        result = propertySet.process(&metric->properties);
    }

    isHistorical = metric->has_is_historical && metric->is_historical;
    isTransient = metric->has_is_transient && metric->is_transient;
    hasTimestamp = metric->has_timestamp;
    timestamp = hasTimestamp ? metric->timestamp : 0;

    value = {};
    stringValue.clear();
    isNull = (metric->has_is_null && metric->is_null) || !isSupportedType();

    if (isNull)
    {
        return result;
    }

    switch (type)
    {
    case METRIC_DATA_TYPE_INT8:
    case METRIC_DATA_TYPE_UINT8:
    case METRIC_DATA_TYPE_INT16:
    case METRIC_DATA_TYPE_UINT16:
    case METRIC_DATA_TYPE_INT32:
    case METRIC_DATA_TYPE_UINT32:
        // Implementations differ on which field carries UInt32, so accept either
        integerTag = metric->which_value;
        value.longValue = metric->which_value == org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag
                              ? metric->value.long_value
                              : metric->value.int_value;
        break;
    case METRIC_DATA_TYPE_INT64:
    case METRIC_DATA_TYPE_UINT64:
    case METRIC_DATA_TYPE_DATETIME:
        value.longValue = metric->value.long_value;
        break;
    case METRIC_DATA_TYPE_FLOAT:
        value.floatValue = metric->value.float_value;
        break;
    case METRIC_DATA_TYPE_DOUBLE:
        value.doubleValue = metric->value.double_value;
        break;
    case METRIC_DATA_TYPE_BOOLEAN:
        value.booleanValue = metric->value.boolean_value;
        break;
    case METRIC_DATA_TYPE_STRING:
    case METRIC_DATA_TYPE_TEXT:
        if (metric->value.string_value)
        {
            stringValue = metric->value.string_value;
        }
        else
        {
            isNull = true;
        }
        break;
    default:
        break;
    }

    return result;
}

bool Metric::isDirty() const
{
    return dirty;
}

uint32_t Metric::getType() const
{
    return type;
}
