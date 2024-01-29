/*
 * File: TahuTypes.cpp
 * Project: cpp_sparkplug_host
 * Created Date: Thursday December 21st 2023
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

/*
 * File: TahuTypes.cpp
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

#include "TahuTypes.h"
#include <cstdio>

#define DEBUGGING 1
#ifdef DEBUGGING
#define LOGGER(format, ...) \
    printf("TahuTypes: ");  \
    printf(format, ##__VA_ARGS__)
#else
#define LOGGER(out, ...)
#endif

tahu::Metric *findMetric(tahu::Payload *base, char *name)
{
    for (size_t i = 0; i < base->metrics_count; i++)
    {
        if (strcmp(base->metrics[i].name, name) == 0)
        {
            return &base->metrics[i];
        }
    }
    return nullptr;
}

tahu::Property *findProperty(tahu::PropertySet *base, char *key)
{
    for (size_t i = 0; i < base->keys_count; i++)
    {
        if (strcmp(base->keys[i], key) == 0)
        {
            return &base->values[i];
        }
    }
    return nullptr;
}

bool copyProperty(tahu::Property *destination, tahu::Property *source)
{
    if (destination->type != source->type)
    {
        return false;
    }
    if (destination->type == PROPERTY_DATA_TYPE_STRING || destination->type == PROPERTY_DATA_TYPE_TEXT)
    {
        if (!destination->is_null)
        {
            free(destination->value.string_value);
        }

        destination->value.string_value = strdup(source->value.string_value);
        return true;
    }

    set_propertyvalue(destination, source->type, (void *)&source->value, 0);
    return true;
}

bool copyProperties(tahu::PropertySet *destination, tahu::PropertySet *source)
{
    for (size_t i = 0; i < source->keys_count; i++)
    {

        tahu::Property *property = findProperty(destination, source->keys[i]);
        if (property == nullptr)
        {
            tahu::Property *sourceProperty = &source->values[i];
            if (sourceProperty->type == PROPERTY_DATA_TYPE_STRING || sourceProperty->type == PROPERTY_DATA_TYPE_TEXT)
            {
                add_property_to_set(
                    destination,
                    source->keys[i],
                    sourceProperty->type,
                    &sourceProperty->value,
                    strlen(sourceProperty->value.string_value));
            }
            else
            {

                add_property_to_set(destination, source->keys[i], sourceProperty->type, &sourceProperty->value, 0);
            }
        }
        if (!copyProperty(property, &source->values[i]))
        {
            return false;
        }
    }
    return true;
}

bool copyMetric(tahu::Metric *destination, tahu::Metric *source)
{
    /*
            char *name;
        bool has_alias;
        uint64_t alias;
        bool has_timestamp;
        uint64_t timestamp;
        bool has_datatype;
        uint32_t datatype;
        bool has_is_historical;
        bool is_historical;
        bool has_is_transient;
        bool is_transient;
        bool has_is_null;
        bool is_null;
        bool has_metadata;
        org_eclipse_tahu_protobuf_Payload_MetaData metadata;
        bool has_properties;
        org_eclipse_tahu_protobuf_Payload_PropertySet properties;
        pb_size_t which_value;
     */

    destination->has_timestamp = source->has_timestamp;
    destination->timestamp = source->timestamp;
    destination->has_datatype = source->has_datatype;
    destination->datatype = source->datatype;
    destination->has_is_historical = source->has_is_historical;
    destination->is_historical = source->is_historical;
    destination->has_is_transient = source->has_is_transient;
    destination->is_transient = source->is_transient;
    destination->has_is_null = source->has_is_null;
    destination->is_null = source->is_null;
    destination->has_metadata = source->has_metadata;
    destination->has_properties |= source->has_properties;
    destination->which_value = source->which_value;
    destination->value = source->value;

    if (source->has_properties)
    {
        if (!destination->has_properties)
        {
            // Initialize Properties
        }
        copyProperties(&destination->properties, &source->properties);
    }

    return false;
}

bool appendPayload(tahu::Payload *base, tahu::Payload *input, bool isBirth)
{
    base->has_timestamp = input->has_timestamp;
    base->timestamp = input->timestamp;
    base->has_seq = input->has_seq;
    base->seq = input->seq;

    if (base->uuid != nullptr)
    {
        free(base->uuid);
        base->uuid = nullptr;
    }

    if (input->uuid != nullptr)
    {
        base->uuid = strdup(input->uuid);
    }

    for (size_t i = 0; i < input->metrics_count; i++)
    {
        tahu::Metric *metric;
        if (!isBirth)
        {
            metric = findMetric(base, input->metrics[i].name);

            if (metric == nullptr)
            {
                // tahu::Metric not found, invalid.
                // Should rebirth
                return false;
            }
        }
        else
        {
            metric = (tahu::Metric *)malloc(sizeof(tahu::Metric));
            memset(metric, 0, sizeof(tahu::Metric));
            metric->name = strdup(input->metrics[i].name);
            add_metric_to_payload(base, metric);
        }
        copyMetric(metric, &input->metrics[i]);
    }

    return true;
}