/*
 * File: Logger.cpp
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

#include "LogMacros.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <spdlog/cfg/argv.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{
    const char *const LOGGER_NAME = "cpp_sparkplug_host";
    const char *const LEVEL_VARIABLE = "CPP_SPARKPLUG_HOST_LOG_LEVEL";
    const char *const PATTERN = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";

    spdlog::level::level_enum toSpdlog(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warning:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        default:
            return spdlog::level::off;
        }
    }

    LogLevel fromSpdlog(spdlog::level::level_enum level)
    {
        switch (level)
        {
        case spdlog::level::trace:
            return LogLevel::Trace;
        case spdlog::level::debug:
            return LogLevel::Debug;
        case spdlog::level::info:
            return LogLevel::Info;
        case spdlog::level::warn:
            return LogLevel::Warning;
        case spdlog::level::err:
        case spdlog::level::critical:
            return LogLevel::Error;
        default:
            return LogLevel::None;
        }
    }

    void applyLevelName(spdlog::logger &logger, const char *name)
    {
        if (name && *name)
        {
            logger.set_level(spdlog::level::from_str(name));
        }
    }

    struct LoggerState
    {
        // All output goes through a dist sink, whose child sinks can be swapped safely while logging
        std::shared_ptr<spdlog::sinks::dist_sink_mt> output = std::make_shared<spdlog::sinks::dist_sink_mt>();
        std::shared_ptr<spdlog::logger> logger;

        LoggerState()
        {
            output->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());

            logger = spdlog::get(LOGGER_NAME);
            if (!logger)
            {
                logger = std::make_shared<spdlog::logger>(LOGGER_NAME, output);
                logger->set_pattern(PATTERN);
                logger->set_level(spdlog::level::warn);
                spdlog::register_logger(logger);
            }

            // SPDLOG_LEVEL first, then the library's own variable, which wins
            spdlog::cfg::load_env_levels();
            applyLevelName(*logger, std::getenv(LEVEL_VARIABLE));
        }
    };

    LoggerState &state()
    {
        static LoggerState instance;
        return instance;
    }
}

spdlog::logger *sph::detail::logger()
{
    return state().logger.get();
}

void Logger::setLevel(LogLevel level)
{
    state().logger->set_level(toSpdlog(level));
}

LogLevel Logger::compiledLevel()
{
    return fromSpdlog(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
}

LogLevel Logger::getLevel()
{
    return fromSpdlog(state().logger->level());
}

void Logger::setSink(Sink sink)
{
    auto &current = state();
    if (sink)
    {
        current.output->set_sinks({std::make_shared<spdlog::sinks::callback_sink_mt>(
            [sink = std::move(sink)](const spdlog::details::log_msg &message)
            { sink(fromSpdlog(message.level), std::string(message.payload.data(), message.payload.size())); })});
    }
    else
    {
        auto console = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console->set_pattern(PATTERN);
        current.output->set_sinks({console});
    }
}

void Logger::loadArgv(int argc, const char *const *argv)
{
    auto &current = state();
    // spdlog only reads argv, despite the non-const signature
    spdlog::cfg::load_argv_levels(argc, const_cast<const char **>(argv));

    std::string prefix = std::string(LEVEL_VARIABLE) + "=";
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] && strncmp(argv[i], prefix.c_str(), prefix.size()) == 0)
        {
            applyLevelName(*current.logger, argv[i] + prefix.size());
        }
    }
}
