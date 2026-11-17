/*
 * File: tck_host.cpp
 * Project: cpp_sparkplug_host
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
 */

/*
 * Host Application for the Sparkplug TCK host profile, built only from the library's public API.
 *
 * Runs one SparkplugHost per --broker (all with the same Host ID), and follows Node and Device state
 * through getPayloads(), the way an application would. tck/run_tck.py drives it over stdin, one
 * request per line, and reads one reply line per request from stdout:
 *
 *   rebirth <node>                 requestRebirth()                         -> OK
 *   ncmd <node>                    commandNode() writing a metric from the NBIRTH -> OK <metric> | ERROR <why>
 *   drebirth <node> <device>       commandDevice() with Device Control/Rebirth   -> OK
 *   dcmd <node> <device>           commandDevice() writing a metric from the DBIRTH -> OK <metric> | ERROR <why>
 *   offline <node>[/<device>]      waits up to 5 s for a DEATH update            -> PASS | FAIL <why>
 *   stop                           stops the hosts (publishes the offline STATE) -> OK
 *
 * The library logs to stderr, so stdout carries only the replies.
 */

#include "SparkplugHost.h"
#include "utilities/Logger.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
    std::atomic<bool> stopRequested = false;

    const auto POLL_INTERVAL = std::chrono::milliseconds(20);
    const auto OFFLINE_WAIT = std::chrono::seconds(5);
    const auto BIRTH_WAIT = std::chrono::seconds(5);

    void onSignal(int)
    {
        stopRequested = true;
    }

    struct Options
    {
        std::vector<std::string> brokers;
        std::string hostId = "TckHost";
        std::string clientId = "tck_host";
        std::string group = "Test";
        double stopAfter = 0;
    };

    /** A metric from a BIRTH that a command can write: its name, type and last value */
    struct WritableMetric
    {
        std::string name;
        uint32_t datatype = 0;
        pb_size_t which = 0;
        decltype(tahu::Metric::value) value{};
        std::string stringValue;
    };

    /** What the application knows about one Node or Device, from getPayloads() */
    struct Entity
    {
        bool online = false;
        bool seenBirth = false;
        std::vector<WritableMetric> writable;
    };

    void usage(const char *name)
    {
        std::cerr << "Usage: " << name
                  << " --broker <url> [--broker <url2>] [--host-id TckHost] [--client-id tck_host]"
                     " [--group Test] [--stop-after <seconds>]\n";
    }

    std::optional<Options> parseOptions(int argc, char **argv)
    {
        Options options;
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            auto next = [&]() -> const char *
            { return i + 1 < argc ? argv[++i] : nullptr; };

            if (arg.find('=') != std::string::npos)
            {
                continue; // a logging option, for Logger::loadArgv()
            }

            const char *value = next();
            if (!value)
            {
                std::cerr << "Missing a value for " << arg << "\n";
                return std::nullopt;
            }
            if (arg == "--broker")
                options.brokers.emplace_back(value);
            else if (arg == "--host-id")
                options.hostId = value;
            else if (arg == "--client-id")
                options.clientId = value;
            else if (arg == "--group")
                options.group = value;
            else if (arg == "--stop-after")
                options.stopAfter = std::atof(value);
            else
            {
                std::cerr << "Unknown option " << arg << "\n";
                return std::nullopt;
            }
        }
        if (options.brokers.empty())
        {
            return std::nullopt;
        }
        return options;
    }

    bool isWritable(const tahu::Metric &metric)
    {
        if (!metric.name || (metric.has_is_null && metric.is_null))
        {
            return false;
        }
        std::string name = metric.name;
        // Not application data: the session counter, control metrics and fixed properties
        if (name == "bdSeq" || name.find("Control/") != std::string::npos || name.rfind("Properties/", 0) == 0)
        {
            return false;
        }
        switch (metric.which_value)
        {
        case org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag:
        case org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag:
        case org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag:
        case org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag:
        case org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag:
        case org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag:
            return true;
        default:
            return false;
        }
    }

    std::vector<WritableMetric> writableMetrics(const tahu::Payload *payload)
    {
        std::vector<WritableMetric> metrics;
        for (size_t i = 0; payload && i < payload->metrics_count; i++)
        {
            const tahu::Metric &metric = payload->metrics[i];
            if (!isWritable(metric))
            {
                continue;
            }
            WritableMetric writable;
            writable.name = metric.name;
            writable.datatype = metric.datatype;
            writable.which = metric.which_value;
            if (metric.which_value == org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag)
            {
                writable.stringValue = metric.value.string_value ? metric.value.string_value : "";
            }
            else
            {
                writable.value = metric.value;
            }
            metrics.push_back(std::move(writable));
        }
        return metrics;
    }

    /**
     * @brief A command payload that writes a new value to one metric, with the datatype from the birth
     */
    tahu::PayloadPtr writeCommand(const WritableMetric &metric)
    {
        tahu::PayloadPtr payload = tahu::makePayload();
        // A null value adds the metric with its name and datatype; the value is filled in below
        add_simple_metric(payload.get(), metric.name.c_str(), false, 0, metric.datatype, false, false, nullptr, 0);
        tahu::Metric &out = payload->metrics[payload->metrics_count - 1];
        out.has_is_null = false;
        out.is_null = false;
        out.which_value = metric.which;

        switch (metric.which)
        {
        case org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag:
            out.value.boolean_value = !metric.value.boolean_value;
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag:
            // Small enough to fit every integer type, including Int8 and UInt8
            out.value.int_value = (metric.value.int_value + 1) & 0x3F;
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag:
            out.value.long_value = (metric.value.long_value + 1) & 0x3F;
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_float_value_tag:
            out.value.float_value = metric.value.float_value + 1.0f;
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_double_value_tag:
            out.value.double_value = metric.value.double_value + 1.0;
            break;
        case org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag:
            out.value.string_value = strdup((metric.stringValue + " (written by tck_host)").c_str());
            break;
        default:
            break;
        }
        return payload;
    }

    /**
     * @brief The application side: hosts, their threads, and the Node and Device state seen so far
     */
    class TckHost
    {
    private:
        Options options;
        std::vector<std::unique_ptr<SparkplugHost>> hosts;
        std::vector<std::thread> threads;
        std::map<std::string, Entity> entities;

    public:
        explicit TckHost(Options options) : options(std::move(options)) {}

        ~TckHost()
        {
            stop();
        }

        void start()
        {
            for (size_t i = 0; i < options.brokers.size(); i++)
            {
                std::string clientId = options.clientId + (i == 0 ? "" : "_" + std::to_string(i + 1));
                hosts.push_back(std::make_unique<SparkplugHost>(options.brokers[i], clientId, options.hostId));
            }
            for (auto &host : hosts)
            {
                SparkplugHost *instance = host.get();
                threads.emplace_back([instance]()
                                     { instance->run(); });
            }
        }

        void stop()
        {
            for (auto &host : hosts)
            {
                host->stop();
            }
            for (auto &thread : threads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
            threads.clear();
            // Destroying a host disconnects it, after run() has published the offline STATE
            hosts.clear();
        }

        /** Applies the updates since the last call. Commands go through the first broker's host. */
        void poll()
        {
            if (hosts.empty())
            {
                return;
            }
            for (PublishableUpdate &update : hosts.front()->getPayloads())
            {
                Entity &entity = entities[update.id];
                switch (update.type)
                {
                case UpdateType::BIRTH:
                    entity.online = true;
                    entity.seenBirth = true;
                    entity.writable = writableMetrics(update.payload.get());
                    std::cerr << "tck_host: BIRTH " << update.id << " (" << entity.writable.size()
                              << " writable metrics)" << std::endl;
                    break;
                case UpdateType::DEATH:
                    entity.online = false;
                    std::cerr << "tck_host: DEATH " << update.id << std::endl;
                    break;
                case UpdateType::PUBLISH:
                    break;
                }
            }
        }

        std::string handle(const std::string &line)
        {
            std::istringstream input(line);
            std::string request, node, device;
            input >> request >> node >> device;

            if (request == "rebirth" && !node.empty())
            {
                hosts.front()->requestRebirth(options.group, node);
                return "OK";
            }
            if (request == "ncmd" && !node.empty())
            {
                return writeMetric(node, [&](tahu::PayloadPtr payload)
                                   { hosts.front()->commandNode(options.group, node, std::move(payload)); });
            }
            if (request == "drebirth" && !device.empty())
            {
                // Sparkplug has no Device rebirth: a Device rebirths with its Node. This exercises
                // the DCMD path with a boolean control metric, as the TCK's prompt asks for.
                tahu::PayloadPtr payload = tahu::makePayload();
                bool value = true;
                add_simple_metric(payload.get(), "Device Control/Rebirth", false, 0, METRIC_DATA_TYPE_BOOLEAN,
                                  false, false, &value, sizeof(value));
                hosts.front()->commandDevice(options.group, node, device, std::move(payload));
                return "OK";
            }
            if (request == "dcmd" && !device.empty())
            {
                return writeMetric(node + "/" + device, [&](tahu::PayloadPtr payload)
                                   { hosts.front()->commandDevice(options.group, node, device, std::move(payload)); });
            }
            if (request == "offline" && !node.empty())
            {
                return waitOffline(node);
            }
            if (request == "stop")
            {
                stopRequested = true;
                return "OK";
            }
            return "ERROR unknown request: " + line;
        }

    private:
        /** PublishableUpdate ids are <group>/<node> and <group>/<node>/<device> */
        std::string updateId(const std::string &path) const
        {
            return options.group + "/" + path;
        }

        template <typename Send>
        std::string writeMetric(const std::string &path, Send send)
        {
            const std::string id = updateId(path);
            // The TCK prompts for a command as soon as it sees the birth, which can be before the host
            // has handed that birth to the application. Wait for it.
            auto deadline = std::chrono::steady_clock::now() + BIRTH_WAIT;
            auto entity = entities.end();
            while (true)
            {
                poll();
                entity = entities.find(id);
                if (entity != entities.end() && entity->second.seenBirth && entity->second.online)
                {
                    break;
                }
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    return "ERROR no birth seen for " + id;
                }
                std::this_thread::sleep_for(POLL_INTERVAL);
            }
            if (entity->second.writable.empty())
            {
                return "ERROR no writable metric in the birth of " + id;
            }
            // Prefer a boolean, the simplest value to write back
            const WritableMetric *chosen = &entity->second.writable.front();
            for (const auto &metric : entity->second.writable)
            {
                if (metric.which == org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag)
                {
                    chosen = &metric;
                    break;
                }
            }
            send(writeCommand(*chosen));
            return "OK " + chosen->name;
        }

        /**
         * @brief Answers the TCK's "mark it offline" and "mark its metrics stale" prompts. The library
         * reports both the same way: a DEATH update, after which none of the entity's metrics are
         * current until its next BIRTH.
         */
        std::string waitOffline(const std::string &path)
        {
            const std::string id = updateId(path);
            auto deadline = std::chrono::steady_clock::now() + OFFLINE_WAIT;
            while (true)
            {
                poll();
                auto entity = entities.find(id);
                if (entity != entities.end() && entity->second.seenBirth && !entity->second.online)
                {
                    return "PASS";
                }
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    if (entity == entities.end() || !entity->second.seenBirth)
                    {
                        return "FAIL no birth seen for " + id;
                    }
                    return "FAIL " + id + " is still online";
                }
                std::this_thread::sleep_for(POLL_INTERVAL);
            }
        }
    };
}

int main(int argc, char **argv)
{
    auto options = parseOptions(argc, argv);
    if (!options)
    {
        usage(argv[0]);
        return 2;
    }

    if (!getenv("CPP_SPARKPLUG_HOST_LOG_LEVEL") && !getenv("SPDLOG_LEVEL"))
    {
        Logger::setLevel(LogLevel::Debug);
    }
    Logger::loadArgv(argc, argv);

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    // Requests from the runner arrive on stdin; read them on their own thread so polling never blocks
    std::mutex requestLock;
    std::deque<std::string> requests;
    std::thread reader([&]()
                       {
        std::string line;
        while (std::getline(std::cin, line))
        {
            std::lock_guard<std::mutex> guard(requestLock);
            requests.push_back(line);
        } });
    reader.detach();

    TckHost host(*options);
    host.start();

    auto started = std::chrono::steady_clock::now();
    while (!stopRequested)
    {
        if (options->stopAfter > 0 &&
            std::chrono::steady_clock::now() - started >= std::chrono::duration<double>(options->stopAfter))
        {
            std::cerr << "tck_host: --stop-after reached, stopping" << std::endl;
            break;
        }

        host.poll();

        std::deque<std::string> pending;
        {
            std::lock_guard<std::mutex> guard(requestLock);
            pending.swap(requests);
        }
        for (const auto &request : pending)
        {
            std::string reply = host.handle(request);
            std::cerr << "tck_host: " << request << " -> " << reply << std::endl;
            std::cout << reply << std::endl;
        }

        std::this_thread::sleep_for(POLL_INTERVAL);
    }

    host.stop();
    std::cerr << "tck_host: stopped" << std::endl;
    // The stdin reader may still be blocked in getline; leave without waiting for it
    std::_Exit(0);
}
