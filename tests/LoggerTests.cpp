#include "Payloads.h"
#include "Test.h"
#include "utilities/Logger.h"
#include <string>
#include <vector>

namespace
{
    struct CapturedLog
    {
        std::vector<std::pair<LogLevel, std::string>> messages;

        explicit CapturedLog(LogLevel level)
        {
            Logger::setLevel(level);
            Logger::setSink([this](LogLevel messageLevel, const std::string &message)
                            { messages.emplace_back(messageLevel, message); });
        }

        ~CapturedLog()
        {
            Logger::setSink(nullptr);
            Logger::setLevel(LogLevel::None);
        }

        // Data from an unknown node logs at Info
        static void triggerInfoMessage()
        {
            Group group("G");
            PayloadBuilder builder;
            builder.seq(1).doubleMetric("Temperature", 1);
            send(group, "spBv1.0/G/NDATA/N", builder);
        }
    };
}

// Info messages exist only when the build keeps them (CPP_SPARKPLUG_HOST_LOG_LEVEL at info or more verbose)
static bool infoCompiledIn()
{
    return Logger::compiledLevel() <= LogLevel::Info;
}

TEST(logger_sink_receives_messages_at_level)
{
    if (!infoCompiledIn())
        return;
    CapturedLog log(LogLevel::Info);
    CapturedLog::triggerInfoMessage();
    CHECK_EQ(log.messages.size(), size_t(1));
    CHECK(log.messages[0].first == LogLevel::Info);
    CHECK_EQ(log.messages[0].second, std::string("Node: Received a message for G/N while it is offline"));
}

TEST(logger_filters_below_level)
{
    CapturedLog log(LogLevel::Warning);
    CapturedLog::triggerInfoMessage();
    CHECK(log.messages.empty());
}

TEST(logger_level_round_trips)
{
    CapturedLog log(LogLevel::Error);
    CHECK(Logger::getLevel() == LogLevel::Error);
    Logger::setLevel(LogLevel::Debug);
    CHECK(Logger::getLevel() == LogLevel::Debug);
}

TEST(logger_reads_level_from_arguments)
{
    CapturedLog log(LogLevel::None);
    const char *argv[] = {"program", "tcp://broker:1883", "CPP_SPARKPLUG_HOST_LOG_LEVEL=info"};
    Logger::loadArgv(3, argv);
    CHECK(Logger::getLevel() == LogLevel::Info);
    CapturedLog::triggerInfoMessage();
    CHECK_EQ(log.messages.size(), size_t(infoCompiledIn() ? 1 : 0));
}

TEST(logger_messages_below_compiled_level_are_removed)
{
    if (infoCompiledIn())
        return;
    CapturedLog log(LogLevel::Trace);
    CapturedLog::triggerInfoMessage();
    CHECK(log.messages.empty());
}
