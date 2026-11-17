/*
 * File: PropertySet.h
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

#ifndef SRC_TYPES_PROPERTYSET
#define SRC_TYPES_PROPERTYSET

#include "TahuTypes.h"
#include "Property.h"
#include "../DataCollection.h"
#include "CommonTypes.h"

/**
 * @brief The last known values of a Sparkplug Property Set, keyed by property name
 *
 */
class PropertySet : public DataCollection<Property>
{
public:
    PropertySet() = default;
    explicit PropertySet(const std::string &) {}

    /**
     * @brief Appends the Property Set to a Metric, if any property changed
     *
     * @param metric
     * @param force Append every property, changed or not
     * @param consume Mark the properties as clean once appended
     */
    void appendTo(tahu::Metric *metric, bool force = false, bool consume = true);
    /**
     * @brief Appends the properties in this Property Set to a tahu Property Set
     *
     * @param propertySet
     * @param force Append every property, changed or not
     * @param consume Mark the properties as clean once appended
     */
    void appendTo(tahu::PropertySet *propertySet, bool force = false, bool consume = true);
    /**
     * @brief Updates the Property Set from a received tahu Property Set
     *
     * @param propertySet
     * @return ParseResult OUT_OF_SYNC if any property changed datatype
     */
    ParseResult process(const tahu::PropertySet *propertySet);
};

#endif /* SRC_TYPES_PROPERTYSET */
