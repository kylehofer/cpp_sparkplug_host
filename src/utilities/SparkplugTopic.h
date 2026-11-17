/*
 * File: SparkplugTopic.h
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

#ifndef SRC_UTILITIES_SPARKPLUGTOPIC
#define SRC_UTILITIES_SPARKPLUGTOPIC

#include <string>

/**
 * @brief The message type part of a Sparkplug topic.
 *
 */
enum class SparkplugCommandType
{
    NBIRTH,
    NDEATH,
    NDATA,
    NCMD,
    DBIRTH,
    DDEATH,
    DDATA,
    DCMD,
    INVALID
};

/**
 * @brief Parses a Sparkplug B topic of the form
 * spBv1.0/<group_id>/<message_type>/<edge_node_id>[/<device_id>]
 *
 * STATE topics are not handled here and fail to parse.
 */
class SparkplugTopic
{
private:
    std::string group;
    std::string node;
    std::string device;
    SparkplugCommandType command = SparkplugCommandType::INVALID;
    bool hasDevice = false;

public:
    /**
     * @brief Parses a topic, replacing any previously parsed values.
     *
     * @param input The full MQTT topic
     * @return true The topic is a valid Sparkplug node or device topic
     * @return false The topic is not a Sparkplug node or device topic
     */
    bool parse(const std::string &input);
    SparkplugCommandType getCommandType() const;
    /** @brief NBIRTH or DBIRTH */
    bool isBirth() const;
    /** @brief NDEATH or DDEATH */
    bool isDeath() const;
    /** @brief NDATA or DDATA */
    bool isData() const;
    /** @brief NCMD or DCMD */
    bool isCommand() const;
    /** @brief Whether the topic contains a device id */
    bool isDevice() const;
    const std::string &getGroup() const;
    const std::string &getNode() const;
    /** @brief The device id, or an empty string for node topics */
    const std::string &getDevice() const;
};

#endif /* SRC_UTILITIES_SPARKPLUGTOPIC */
