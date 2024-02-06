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
#include <string>

class PropertySet;

/**
 * @brief A class that represents a Sparkplug Property
 *
 */
class Property
{
private:
    size_t length;
    uint32_t type;
    bool dirty;
    std::string name;

    union
    {
        uint32_t intValue;
        uint64_t longValue;
        float floatValue;
        double doubleValue;
        bool booleanValue;
        char *stringValue;
        PropertySet *propertySetValue;
    } value;

    /**
     * @brief Clears the value on the Property
     * Frees any allocated memory
     */
    inline void clearValue();

protected:
public:
    Property(std::string &name);
    ~Property();
    /**
     * @brief Clears the data associated with this Property
     *
     */
    void clear();
    /**
     * @brief Appends this property to a Property Set if it has had changes
     *
     * @param propertySet
     * @param force Forces the Property to be appended
     */
    void appendTo(tahu::PropertySet *propertySet, bool force = false);
    /**
     * @brief Processes a tahu Property and updates the Property
     *
     * @param property
     * @return ParseResult
     */
    ParseResult process(tahu::Property *property);
    /**
     * @brief Whether the property has data that hasn't been acknowledged
     *
     * @return true
     * @return false
     */
    bool isDirty();
};

#endif /* SRC_TYPES_PROPERTY */
