/*
 * File: Logger.h
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

#ifndef SRC_UTILITIES_LOGGER
#define SRC_UTILITIES_LOGGER

#include <functional>
#include <string>

/**
 * @brief Severity of a log message. Messages below the configured level are discarded.
 *
 */
enum class LogLevel
{
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    None
};

/**
 * @brief Controls the library's logging.
 *
 * The library logs through an spdlog logger named "cpp_sparkplug_host". By default, messages at
 * Warning and above go to stderr. The level can be set in four ways (later ones win):
 *
 * - Build time: -DCPP_SPARKPLUG_HOST_LOG_LEVEL=<level> sets the most verbose level compiled in.
 *   Calls below it are removed from the binary and cannot be enabled at run time. Defaults to debug.
 * - Environment: SPDLOG_LEVEL (e.g. "cpp_sparkplug_host=debug") or
 *   CPP_SPARKPLUG_HOST_LOG_LEVEL=debug, read when the library first logs.
 * - Command line: Logger::loadArgv() with arguments such as CPP_SPARKPLUG_HOST_LOG_LEVEL=debug.
 * - Code: Logger::setLevel().
 *
 * Level names: trace, debug, info, warn, error, critical, off.
 *
 * Every function is thread safe. The application does not need to use spdlog itself.
 *
 * @code
 * Logger::setLevel(LogLevel::Debug);
 * Logger::setSink([](LogLevel level, const std::string &message) {
 *     myLogger.write(static_cast<int>(level), message);
 * });
 * @endcode
 */
class Logger
{
public:
    /**
     * @brief Receives every log message at or above the configured level.
     * The message is "<component>: <text>", without a timestamp or trailing newline.
     */
    using Sink = std::function<void(LogLevel level, const std::string &message)>;

    /**
     * @brief Sets the minimum level that is logged. Use LogLevel::None to silence the library.
     * Levels more verbose than the build-time level have no effect.
     *
     * @param level
     */
    static void setLevel(LogLevel level);

    /**
     * @brief Gets the minimum level that is logged.
     *
     * @return LogLevel
     */
    static LogLevel getLevel();

    /**
     * @brief The most verbose level compiled into the library (CPP_SPARKPLUG_HOST_LOG_LEVEL at build time).
     * Messages more verbose than this are never logged, whatever setLevel() says.
     *
     * @return LogLevel
     */
    static LogLevel compiledLevel();

    /**
     * @brief Sends log messages to a callback instead of stderr. Pass nullptr to restore stderr.
     *
     * @param sink
     */
    static void setSink(Sink sink);

    /**
     * @brief Applies log levels given on the command line. Recognises
     * CPP_SPARKPLUG_HOST_LOG_LEVEL=<level> and spdlog's SPDLOG_LEVEL=<levels>. Other arguments are ignored.
     *
     * @param argc
     * @param argv
     */
    static void loadArgv(int argc, const char *const *argv);
};

#endif /* SRC_UTILITIES_LOGGER */
