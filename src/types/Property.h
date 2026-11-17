/*
 * File: Property.h
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

#ifndef SRC_TYPES_PROPERTY
#define SRC_TYPES_PROPERTY

#include "TahuTypes.h"
#include "CommonTypes.h"
#include <memory>
#include <string>

class PropertySet;

/**
 * @brief The last known value of a single Sparkplug metric property.
 *
 * Supports the scalar property types, strings and nested property sets.
 * Property set lists are not supported and are reported as null.
 */
class Property
{
private:
    std::string name;
    uint32_t type = PROPERTY_DATA_TYPE_UNKNOWN;
    bool dirty = false;
    bool isNull = true;

    union PropertyValueUnion
    {
        uint32_t intValue;
        uint64_t longValue;
        float floatValue;
        double doubleValue;
        bool booleanValue;
    } value{};

    std::string stringValue;
    std::unique_ptr<PropertySet> propertySetValue;

public:
    explicit Property(const std::string &name);
    ~Property();
    Property(const Property &) = delete;
    Property &operator=(const Property &) = delete;

    /**
     * @brief Resets the property to an unknown, null value
     *
     */
    void clear();
    /**
     * @brief Appends this property to a Property Set if it has had changes
     *
     * @param propertySet
     * @param force Append the property even if it has not changed
     * @param consume Mark the property as clean once appended
     */
    void appendTo(tahu::PropertySet *propertySet, bool force = false, bool consume = true);
    /**
     * @brief Updates the Property from a received tahu Property
     *
     * @param property
     * @return ParseResult OUT_OF_SYNC if the datatype changed since the birth
     */
    ParseResult process(const tahu::Property *property);
    /**
     * @brief Whether the property has changed since it was last appended
     *
     * @return true
     * @return false
     */
    bool isDirty() const;
};

#endif /* SRC_TYPES_PROPERTY */
