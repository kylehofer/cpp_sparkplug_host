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

using namespace std;

void Property::appendTo(tahu::PropertySet *propertySet, bool force)
{
    if (!dirty && !force)
    {
        return;
    }

    if (!force)
    {
        dirty = false;
    }

    if (type == PROPERTY_DATA_TYPE_PROPERTYSET)
    {
        tahu::PropertySet nestedSet;
        memset(&nestedSet, 0, sizeof(tahu::PropertySet));

        value.propertySetValue->appendTo(&nestedSet, force);
        add_property_to_set(propertySet, name.c_str(), type, &nestedSet, 0);

        // We're allocating memory during appendTo, and the data is duplicated from
        // the nestedSet to the propertySet. Because of this we must release the nestedSet
        pb_release(org_eclipse_tahu_protobuf_Payload_PropertySet_fields, &nestedSet);
    }
    else
    {
        add_property_to_set(propertySet, name.c_str(), type, &value, length);
    }
}

inline void Property::clearValue()
{
    switch (type)
    {
    case PROPERTY_DATA_TYPE_STRING:
    case PROPERTY_DATA_TYPE_TEXT:
        free(value.stringValue);
        break;
    case PROPERTY_DATA_TYPE_PROPERTYSET:
        delete value.propertySetValue;
        break;
    default:
        break;
    }
    memset(&value, 0, sizeof(PropertyValueUnion));
}

ParseResult Property::process(tahu::Property *property)
{
    if (property->type != type && type != PROPERTY_DATA_TYPE_UNKNOWN)
    {
        return ParseResult::OUT_OF_SYNC;
    }

    dirty = true;

    switch (property->type)
    {
    case PROPERTY_DATA_TYPE_INT8:
    case PROPERTY_DATA_TYPE_UINT8:
        length = sizeof(int8_t);
        value.intValue = property->value.int_value;
        break;
    case PROPERTY_DATA_TYPE_INT16:
    case PROPERTY_DATA_TYPE_UINT16:
        length = sizeof(int16_t);
        value.intValue = property->value.int_value;
        break;
    case PROPERTY_DATA_TYPE_INT32:
    case PROPERTY_DATA_TYPE_UINT32:
        length = sizeof(int32_t);
        value.intValue = property->value.int_value;
        break;
    case PROPERTY_DATA_TYPE_INT64:
    case PROPERTY_DATA_TYPE_UINT64:
    case PROPERTY_DATA_TYPE_DATETIME:
        length = sizeof(int64_t);
        value.longValue = property->value.long_value;
        break;
    case PROPERTY_DATA_TYPE_FLOAT:
        length = sizeof(float);
        value.floatValue = property->value.float_value;
        break;
    case PROPERTY_DATA_TYPE_DOUBLE:
        length = sizeof(double);
        value.doubleValue = property->value.double_value;
        break;
    case PROPERTY_DATA_TYPE_BOOLEAN:
        length = sizeof(bool);
        value.booleanValue = property->value.boolean_value;
        break;
    case PROPERTY_DATA_TYPE_STRING:
    case PROPERTY_DATA_TYPE_TEXT:
        if (value.stringValue)
        {
            free(value.stringValue);
        }
        length = strlen(property->value.string_value);
        value.stringValue = strdup(property->value.string_value);
        break;
    case PROPERTY_DATA_TYPE_PROPERTYSET:
        if (!value.propertySetValue)
        {
            value.propertySetValue = new PropertySet();
        }
        value.propertySetValue->process(&property->value.propertyset_value);
        length = sizeof(PropertySet);
        break;
    default:
        break;
    }

    type = property->type;

    return ParseResult::OK;
}

bool Property::isDirty()
{
    return dirty;
}

void Property::clear()
{
    clearValue();
    type = PROPERTY_DATA_TYPE_UNKNOWN;
}

Property::Property(string &name)
{
    clearValue();
    this->name = string(name);
}

Property::~Property()
{
    clear();
}
