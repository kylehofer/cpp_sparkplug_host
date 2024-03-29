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

class SparkplugTopic
{
private:
    std::string group;
    std::string node;
    SparkplugCommandType command;
    std::string device;
    bool hasDevice;

    inline size_t nextSplitter(std::string &input, size_t start);
    inline size_t readPart(std::string &input, size_t start, std::string &output);

    inline SparkplugCommandType parseCommand(std::string &input);

protected:
public:
    bool parse(std::string &input);
    SparkplugCommandType getCommandType();
    bool isBirth();
    bool isDeath();
    bool isData();
    bool isCommand();
    bool isTarget(std::string target);
    bool isDevice();
    std::string &getGroup();
    std::string &getNode();
    std::string &getDevice();
};

#endif /* SRC_UTILITIES_SPARKPLUGTOPIC */
