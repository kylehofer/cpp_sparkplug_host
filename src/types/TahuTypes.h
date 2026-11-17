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
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Shorter aliases for the nanopb generated Tahu types.
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
    typedef org_eclipse_tahu_protobuf_Payload_DataSet DataSet;
    typedef org_eclipse_tahu_protobuf_Payload_Template Template;
    typedef org_eclipse_tahu_protobuf_Payload_Metric_MetricValueExtension MetricValueExtension;

    /**
     * @brief Deleter that releases a heap allocated Payload and everything it owns.
     * Calls free_payload() followed by free().
     */
    struct PayloadDeleter
    {
        void operator()(Payload *payload) const;
    };

    /**
     * @brief Owning pointer to a Tahu Payload.
     * The Payload must have been allocated with malloc/calloc (see makePayload()).
     */
    using PayloadPtr = std::unique_ptr<Payload, PayloadDeleter>;

    /**
     * @brief Allocates a zero initialised Payload.
     *
     * @return PayloadPtr An owned, empty payload
     */
    PayloadPtr makePayload();

    /**
     * @brief Encodes a payload into a buffer sized to fit it.
     *
     * @param payload The payload to encode
     * @return std::optional<std::vector<uint8_t>> The encoded bytes, or std::nullopt if encoding failed
     */
    std::optional<std::vector<uint8_t>> encode(const Payload *payload);

    /**
     * @brief Decodes a payload from a buffer.
     *
     * @param data Encoded protobuf bytes
     * @param length Number of bytes in data
     * @return PayloadPtr The decoded payload, or nullptr if decoding failed
     */
    PayloadPtr decode(const uint8_t *data, size_t length);

    /**
     * @brief Reads an unsigned integer value out of a metric, whatever integer type it was sent as.
     *
     * @param metric
     * @return std::optional<uint64_t> The value, or std::nullopt if the metric is null or not an integer
     */
    std::optional<uint64_t> readUnsigned(const Metric *metric);
}

#endif /* SRC_TYPES_TAHUTYPES */
