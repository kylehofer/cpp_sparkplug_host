/*
 * basic_host: connects as a Sparkplug Primary Host and prints every change it sees.
 *
 * Usage: basic_host [broker] [host_id] [CPP_SPARKPLUG_HOST_LOG_LEVEL=<level>]
 *   broker   defaults to tcp://localhost:1883
 *   host_id  defaults to example-host
 *   level    trace, debug, info (default here), warn, error, off.
 *            The CPP_SPARKPLUG_HOST_LOG_LEVEL environment variable works too.
 *
 * Stop with Ctrl+C.
 */

#include "SparkplugHost.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

namespace
{
    std::atomic<bool> interrupted = false;

    void printMetric(const tahu::Metric &metric)
    {
        const char *name = metric.name ? metric.name : "(no name)";
        if (metric.has_is_null && metric.is_null)
        {
            printf("    %s = null\n", name);
            return;
        }

        switch (metric.which_value)
        {
        case org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag:
            printf("    %s = %u\n", name, metric.value.int_value);
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag:
            printf("    %s = %llu\n", name, static_cast<unsigned long long>(metric.value.long_value));
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag:
            printf("    %s = %f\n", name, metric.value.float_value);
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag:
            printf("    %s = %f\n", name, metric.value.double_value);
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag:
            printf("    %s = %s\n", name, metric.value.boolean_value ? "true" : "false");
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag:
            printf("    %s = \"%s\"\n", name, metric.value.string_value);
            break;
        default:
            printf("    %s = (unsupported type %u)\n", name, metric.datatype);
            break;
        }
    }

    const char *typeName(UpdateType type)
    {
        switch (type)
        {
        case UpdateType::BIRTH:
            return "BIRTH";
        case UpdateType::DEATH:
            return "DEATH";
        default:
            return "DATA";
        }
    }
}

int main(int argc, char **argv)
{
    // Arguments containing '=' are logging options; the rest are positional
    std::vector<const char *> positional;
    for (int i = 1; i < argc; i++)
    {
        if (!strchr(argv[i], '='))
        {
            positional.push_back(argv[i]);
        }
    }
    const char *broker = positional.size() > 0 ? positional[0] : "tcp://localhost:1883";
    const char *hostId = positional.size() > 1 ? positional[1] : "example-host";

    // Show connection activity unless the environment asked for something else,
    // then let command line options override both
    if (!getenv("CPP_SPARKPLUG_HOST_LOG_LEVEL") && !getenv("SPDLOG_LEVEL"))
    {
        Logger::setLevel(LogLevel::Info);
    }
    Logger::loadArgv(argc, argv);
    std::signal(SIGINT, [](int)
                { interrupted = true; });

    SparkplugHost host(broker, std::string(hostId) + "-client", hostId);

    // run() blocks, so it gets its own thread
    std::thread loop([&host]
                     { host.run(); });

    while (!interrupted)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const auto &update : host.getPayloads())
        {
            printf("%s %s\n", typeName(update.type), update.id.c_str());
            if (update.payload)
            {
                for (size_t i = 0; i < update.payload->metrics_count; i++)
                {
                    printMetric(update.payload->metrics[i]);
                }
            }
        }
        // Updates free their own payloads when they go out of scope
    }

    host.stop();
    loop.join();
    return 0;
}
