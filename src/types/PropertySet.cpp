/*
 * File: PropertySet.cpp
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

#include "PropertySet.h"

void PropertySet::appendTo(tahu::Metric *metric, bool force)
{
    if (size() == 0)
    {
        return;
    }

    bool hasChanges = force || any([](Property *property)
                                   { return property->isDirty(); });

    if (!hasChanges)
    {
        return;
    }

    metric->has_properties = true;
    each([&metric, force](Property *property)
         { property->appendTo(&metric->properties, force); });
}

void PropertySet::appendTo(tahu::PropertySet *propertySet, bool force)
{
    if (size() == 0)
    {
        return;
    }

    bool hasChanges = force || any([](Property *property)
                                   { return property->isDirty(); });

    if (!hasChanges)
    {
        return;
    }

    each([&propertySet, force](Property *property)
         { property->appendTo(propertySet, force); });
}

ParseResult PropertySet::process(tahu::PropertySet *propertySet)
{
    for (size_t i = 0; i < propertySet->keys_count; i++)
    {
        const char *key = propertySet->keys[i];
        auto property = get(key);
        if (property->process(&propertySet->values[i]) == ParseResult::OUT_OF_SYNC)
        {
            return ParseResult::OUT_OF_SYNC;
        };
    }
    return ParseResult::OK;
}
