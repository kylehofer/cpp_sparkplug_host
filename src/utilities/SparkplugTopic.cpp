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

using namespace std;

const string DELIMITER{"/"};
const string SPARKPLUG_ID{"spBv1.0"};

const string NBIRTH{"NBIRTH"};
const string NDEATH{"NDEATH"};
const string NDATA{"NDATA"};
const string NCMD{"NCMD"};
const string DBIRTH{"DBIRTH"};
const string DDEATH{"DDEATH"};
const string DDATA{"DDATA"};
const string DCMD{"DCMD"};

inline size_t SparkplugTopic::nextSplitter(string &input, size_t start)
{
    auto index = input.find(DELIMITER, start);
    if (index == string::npos)
    {
        return string::npos;
    }

    if (index >= input.length() - 1)
    {
        return string::npos;
    }

    return index;
}

inline size_t SparkplugTopic::readPart(string &input, size_t start, string &output)
{
    auto index = nextSplitter(input, start);
    if (index == string::npos)
    {
        output = input.substr(start);
        return string::npos;
    }
    output = input.substr(start, index - start);

    return index + 1;
}

bool SparkplugTopic::parse(std::string &input)
{
    size_t index = 0;
    string temp;

    index = readPart(input, index, temp);

    if (index == string::npos || temp.compare(SPARKPLUG_ID) != 0)
    {
        return false;
    }

    index = readPart(input, index, group);

    if (index == string::npos)
    {
        return false;
    }

    index = readPart(input, index, temp);
    command = parseCommand(temp);

    if (index == string::npos || command == SparkplugCommandType::INVALID)
    {
        return false;
    }

    index = readPart(input, index, node);

    if (index == string::npos)
    {
        hasDevice = false;
        return true;
    }

    device = input.substr(index);
    hasDevice = true;

    return true;
}

SparkplugCommandType SparkplugTopic::getCommandType()
{
    return command;
}

bool SparkplugTopic::isBirth()
{
    return (command == SparkplugCommandType::DBIRTH || command == SparkplugCommandType::NBIRTH);
}

bool SparkplugTopic::isDeath()
{
    return (command == SparkplugCommandType::DDEATH || command == SparkplugCommandType::NDEATH);
}

bool SparkplugTopic::isData()
{
    return (command == SparkplugCommandType::DDATA || command == SparkplugCommandType::NDATA);
}

bool SparkplugTopic::isCommand()
{
    return (command == SparkplugCommandType::DCMD || command == SparkplugCommandType::NCMD);
}

bool SparkplugTopic::isTarget(std::string target)
{
    return target.compare(hasDevice ? device : node) == 0;
}

bool SparkplugTopic::isDevice()
{
    return hasDevice;
}

std::string &SparkplugTopic::getGroup()
{
    return group;
}

std::string &SparkplugTopic::getNode()
{
    return node;
}

std::string &SparkplugTopic::getDevice()
{

    return device;
}

inline SparkplugCommandType SparkplugTopic::parseCommand(std::string &input)
{
    if (DDATA.compare(input) == 0)
    {
        return SparkplugCommandType::DDATA;
    }
    if (NDATA.compare(input) == 0)
    {
        return SparkplugCommandType::NDATA;
    }
    if (DBIRTH.compare(input) == 0)
    {
        return SparkplugCommandType::DBIRTH;
    }
    if (NBIRTH.compare(input) == 0)
    {
        return SparkplugCommandType::NBIRTH;
    }
    if (DDEATH.compare(input) == 0)
    {
        return SparkplugCommandType::DDEATH;
    }
    if (NDEATH.compare(input) == 0)
    {
        return SparkplugCommandType::NDEATH;
    }
    if (DCMD.compare(input) == 0)
    {
        return SparkplugCommandType::DCMD;
    }
    if (NCMD.compare(input) == 0)
    {
        return SparkplugCommandType::NCMD;
    }

    return SparkplugCommandType::INVALID;
}