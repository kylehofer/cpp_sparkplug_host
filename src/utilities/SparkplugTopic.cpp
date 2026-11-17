/*
 * File: SparkplugTopic.cpp
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

#include "SparkplugTopic.h"
#include <vector>

namespace
{
    const char *const SPARKPLUG_ID = "spBv1.0";

    SparkplugCommandType parseCommand(const std::string &input)
    {
        if (input == "NBIRTH")
            return SparkplugCommandType::NBIRTH;
        if (input == "NDEATH")
            return SparkplugCommandType::NDEATH;
        if (input == "NDATA")
            return SparkplugCommandType::NDATA;
        if (input == "NCMD")
            return SparkplugCommandType::NCMD;
        if (input == "DBIRTH")
            return SparkplugCommandType::DBIRTH;
        if (input == "DDEATH")
            return SparkplugCommandType::DDEATH;
        if (input == "DDATA")
            return SparkplugCommandType::DDATA;
        if (input == "DCMD")
            return SparkplugCommandType::DCMD;
        return SparkplugCommandType::INVALID;
    }

    std::vector<std::string> split(const std::string &input)
    {
        std::vector<std::string> parts;
        size_t start = 0;
        while (true)
        {
            size_t index = input.find('/', start);
            if (index == std::string::npos)
            {
                parts.push_back(input.substr(start));
                return parts;
            }
            parts.push_back(input.substr(start, index - start));
            start = index + 1;
        }
    }

    bool isDeviceCommand(SparkplugCommandType command)
    {
        return command == SparkplugCommandType::DBIRTH ||
               command == SparkplugCommandType::DDEATH ||
               command == SparkplugCommandType::DDATA ||
               command == SparkplugCommandType::DCMD;
    }
}

bool SparkplugTopic::parse(const std::string &input)
{
    group.clear();
    node.clear();
    device.clear();
    hasDevice = false;
    command = SparkplugCommandType::INVALID;

    auto parts = split(input);

    if (parts.size() < 4 || parts.size() > 5 || parts[0] != SPARKPLUG_ID)
    {
        return false;
    }

    for (const auto &part : parts)
    {
        if (part.empty())
        {
            return false;
        }
    }

    SparkplugCommandType parsed = parseCommand(parts[2]);
    if (parsed == SparkplugCommandType::INVALID)
    {
        return false;
    }

    // Device message types must carry a device id, node message types must not.
    bool withDevice = parts.size() == 5;
    if (withDevice != isDeviceCommand(parsed))
    {
        return false;
    }

    command = parsed;
    group = parts[1];
    node = parts[3];
    if (withDevice)
    {
        device = parts[4];
        hasDevice = true;
    }

    return true;
}

SparkplugCommandType SparkplugTopic::getCommandType() const
{
    return command;
}

bool SparkplugTopic::isBirth() const
{
    return command == SparkplugCommandType::DBIRTH || command == SparkplugCommandType::NBIRTH;
}

bool SparkplugTopic::isDeath() const
{
    return command == SparkplugCommandType::DDEATH || command == SparkplugCommandType::NDEATH;
}

bool SparkplugTopic::isData() const
{
    return command == SparkplugCommandType::DDATA || command == SparkplugCommandType::NDATA;
}

bool SparkplugTopic::isCommand() const
{
    return command == SparkplugCommandType::DCMD || command == SparkplugCommandType::NCMD;
}

bool SparkplugTopic::isDevice() const
{
    return hasDevice;
}

const std::string &SparkplugTopic::getGroup() const
{
    return group;
}

const std::string &SparkplugTopic::getNode() const
{
    return node;
}

const std::string &SparkplugTopic::getDevice() const
{
    return device;
}
