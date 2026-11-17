/*
 * File: HostState.cpp
 * Project: cpp_sparkplug_host
 * Created Date: Friday November 6th 2026
 * Author: Kyle Hofer
 *
 * MIT License
 *
 * Copyright (c) 2026 Kyle Hofer
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

#include "HostState.h"
#include <cctype>

namespace
{
    // Finds the value that follows "key": in a flat JSON object
    std::optional<size_t> findValue(const std::string &json, const std::string &key)
    {
        std::string quoted = "\"" + key + "\"";
        size_t index = json.find(quoted);
        while (index != std::string::npos)
        {
            size_t cursor = index + quoted.size();
            while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])))
            {
                cursor++;
            }
            if (cursor < json.size() && json[cursor] == ':')
            {
                cursor++;
                while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])))
                {
                    cursor++;
                }
                return cursor;
            }
            index = json.find(quoted, index + 1);
        }
        return std::nullopt;
    }
}

std::optional<HostState> HostState::parse(const std::string &json)
{
    HostState state;

    auto online = findValue(json, "online");
    if (!online)
    {
        return std::nullopt;
    }
    if (json.compare(*online, 4, "true") == 0)
    {
        state.online = true;
    }
    else if (json.compare(*online, 5, "false") == 0)
    {
        state.online = false;
    }
    else
    {
        return std::nullopt;
    }

    auto timestamp = findValue(json, "timestamp");
    if (timestamp)
    {
        uint64_t value = 0;
        for (size_t i = *timestamp; i < json.size() && std::isdigit(static_cast<unsigned char>(json[i])); i++)
        {
            value = value * 10 + static_cast<uint64_t>(json[i] - '0');
        }
        state.timestamp = value;
    }

    return state;
}

std::string HostState::toJson() const
{
    // Spaced exactly as earlier versions wrote it. Edge nodes running cpp_sparkplug from before
    // its tolerant STATE parser look for the substring "online": true (one space after the colon),
    // and take anything else as neither online nor offline. Compact JSON leaves them waiting for
    // a host they never see come online.
    return std::string("{ \"online\": ") + (online ? "true" : "false") +
           ", \"timestamp\": " + std::to_string(timestamp) + " }";
}
