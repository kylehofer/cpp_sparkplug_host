/*
 * File: Property.cpp
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

#include "Property.h"
#include "PropertySet.h"
#include <pb_decode.h>
#include <cstring>

namespace
{
    bool isStringType(uint32_t type)
    {
        return type == PROPERTY_DATA_TYPE_STRING || type == PROPERTY_DATA_TYPE_TEXT;
    }
}

Property::Property(const std::string &name) : name(name)
{
}

Property::~Property() = default;

void Property::appendTo(tahu::PropertySet *propertySet, bool force, bool consume)
{
    if (!dirty && !force)
    {
        return;
    }

    if (consume)
    {
        dirty = false;
    }

    if (isNull || type == PROPERTY_DATA_TYPE_UNKNOWN)
    {
        // A null value makes add_property_to_set flag the property as is_null
        add_property_to_set(propertySet, name.c_str(), type, nullptr, 0);
        return;
    }

    if (type == PROPERTY_DATA_TYPE_PROPERTYSET)
    {
        tahu::PropertySet nestedSet;
        memset(&nestedSet, 0, sizeof(tahu::PropertySet));

        propertySetValue->appendTo(&nestedSet, true, consume);
        add_property_to_set(propertySet, name.c_str(), type, &nestedSet, 0);

        // add_property_to_set deep copies the nested set, so our copy must be released
        pb_release(org_eclipse_tahu_protobuf_Payload_PropertySet_fields, &nestedSet);
    }
    else if (isStringType(type))
    {
        add_property_to_set(propertySet, name.c_str(), type, stringValue.c_str(), stringValue.size());
    }
    else
    {
        add_property_to_set(propertySet, name.c_str(), type, &value, 0);
    }
}

ParseResult Property::process(const tahu::Property *property)
{
    uint32_t incomingType = property->has_type ? property->type : type;

    if (incomingType != type && type != PROPERTY_DATA_TYPE_UNKNOWN)
    {
        return ParseResult::OUT_OF_SYNC;
    }

    dirty = true;
    type = incomingType;
    value = {};
    isNull = property->has_is_null && property->is_null;

    if (isNull)
    {
        stringValue.clear();
        propertySetValue.reset();
        return ParseResult::OK;
    }

    switch (type)
    {
    case PROPERTY_DATA_TYPE_INT8:
    case PROPERTY_DATA_TYPE_UINT8:
    case PROPERTY_DATA_TYPE_INT16:
    case PROPERTY_DATA_TYPE_UINT16:
    case PROPERTY_DATA_TYPE_INT32:
        value.intValue = property->value.int_value;
        break;
    case PROPERTY_DATA_TYPE_UINT32:
        // UInt32 properties are carried in long_value by Tahu, but accept either
        value.longValue = property->which_value == org_eclipse_tahu_protobuf_Payload_PropertyValue_int_value_tag
                              ? property->value.int_value
                              : property->value.long_value;
        break;
    case PROPERTY_DATA_TYPE_INT64:
    case PROPERTY_DATA_TYPE_UINT64:
    case PROPERTY_DATA_TYPE_DATETIME:
        value.longValue = property->value.long_value;
        break;
    case PROPERTY_DATA_TYPE_FLOAT:
        value.floatValue = property->value.float_value;
        break;
    case PROPERTY_DATA_TYPE_DOUBLE:
        value.doubleValue = property->value.double_value;
        break;
    case PROPERTY_DATA_TYPE_BOOLEAN:
        value.booleanValue = property->value.boolean_value;
        break;
    case PROPERTY_DATA_TYPE_STRING:
    case PROPERTY_DATA_TYPE_TEXT:
        if (property->value.string_value)
        {
            stringValue = property->value.string_value;
        }
        else
        {
            stringValue.clear();
            isNull = true;
        }
        break;
    case PROPERTY_DATA_TYPE_PROPERTYSET:
        if (!propertySetValue)
        {
            propertySetValue = std::make_unique<PropertySet>();
        }
        return propertySetValue->process(&property->value.propertyset_value);
    default:
        // Property set lists and unknown types are not supported
        isNull = true;
        break;
    }

    return ParseResult::OK;
}

bool Property::isDirty() const
{
    return dirty;
}

void Property::clear()
{
    type = PROPERTY_DATA_TYPE_UNKNOWN;
    value = {};
    isNull = true;
    stringValue.clear();
    propertySetValue.reset();
}
