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
 * @brief A class that represents a Sparkplug Property Set
 *
 */
class PropertySet : public DataCollection<Property>
{
private:
protected:
public:
    /**
     * @brief Appends the Property Set to the Metric
     *code
     * @param metric
     * @param force Forces all properties to be appended
     */
    void appendTo(tahu::Metric *metric, bool force = false);
    /**
     * @brief Appends all properties in this Property Set to a Property Set
     *
     * @param propertySet
     * @param force Forces all properties to be appended
     */
    void appendTo(tahu::PropertySet *propertySet, bool force = false);
    /**
     * @brief Processes a tahu::PropertySet processing all properties
     *
     * @param propertySet
     * @return ParseResult
     */
    ParseResult process(tahu::PropertySet *propertySet);
};

#endif /* SRC_TYPES_PROPERTYSET */
