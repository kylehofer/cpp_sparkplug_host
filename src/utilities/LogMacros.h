/*
 * File: LogMacros.h
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

#ifndef SRC_UTILITIES_LOGMACROS
#define SRC_UTILITIES_LOGMACROS

/*
 * Internal logging macros. Not installed: this is the only header that includes spdlog.
 *
 * Messages use fmt syntax: SPH_LOG_INFO(COMPONENT, "Connected to {}", address);
 * Calls below SPDLOG_ACTIVE_LEVEL (set from CPP_SPARKPLUG_HOST_LOG_LEVEL) are compiled out.
 */

#include "Logger.h"
#include <spdlog/spdlog.h>

namespace sph::detail
{
    /**
     * @brief The library's logger, created and configured on first use
     */
    spdlog::logger *logger();
}

#define SPH_LOG_TRACE(component, format, ...) \
    SPDLOG_LOGGER_TRACE(::sph::detail::logger(), "{}: " format, component __VA_OPT__(, ) __VA_ARGS__)
#define SPH_LOG_DEBUG(component, format, ...) \
    SPDLOG_LOGGER_DEBUG(::sph::detail::logger(), "{}: " format, component __VA_OPT__(, ) __VA_ARGS__)
#define SPH_LOG_INFO(component, format, ...) \
    SPDLOG_LOGGER_INFO(::sph::detail::logger(), "{}: " format, component __VA_OPT__(, ) __VA_ARGS__)
#define SPH_LOG_WARNING(component, format, ...) \
    SPDLOG_LOGGER_WARN(::sph::detail::logger(), "{}: " format, component __VA_OPT__(, ) __VA_ARGS__)
#define SPH_LOG_ERROR(component, format, ...) \
    SPDLOG_LOGGER_ERROR(::sph::detail::logger(), "{}: " format, component __VA_OPT__(, ) __VA_ARGS__)

#endif /* SRC_UTILITIES_LOGMACROS */
