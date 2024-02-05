/*
 * File: TahuTypes.h
 * Project: cpp_sparkplug_host
 * Created Date: Tuesday December 19th 2023
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

#ifndef SRC_TYPES_TAHUTYPES
#define SRC_TYPES_TAHUTYPES

#include "tahu.h"

/**
 * @brief References Tahu types to an easier to read format
 *
 */
namespace tahu
{
    typedef org_eclipse_tahu_protobuf_Payload Payload;
    typedef org_eclipse_tahu_protobuf_Payload_Metric Metric;
    typedef org_eclipse_tahu_protobuf_Payload_PropertySet PropertySet;
    typedef org_eclipse_tahu_protobuf_Payload_PropertySetList PropertySetList;
    typedef org_eclipse_tahu_protobuf_Payload_PropertyValue_PropertyValueExtension PropertyValueExtension;
    typedef org_eclipse_tahu_protobuf_Payload_PropertyValue Property;
}

bool appendPayload(tahu::Payload *base, tahu::Payload *input, bool isBirth);
tahu::Metric *findMetric(tahu::Payload *base, char *name);

#endif /* SRC_TYPES_TAHUTYPES */
