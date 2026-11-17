/*
 * Helpers for building Sparkplug payloads in tests.
 */

#ifndef TESTS_PAYLOADS
#define TESTS_PAYLOADS

#include "types/Group.h"
#include "types/TahuTypes.h"
#include "utilities/SparkplugTopic.h"
#include <cstring>
#include <string>
#include <vector>

class PayloadBuilder
{
private:
    tahu::PayloadPtr payload = tahu::makePayload();

    PayloadBuilder &add(const char *name, bool hasAlias, uint64_t alias, uint32_t type, const void *value, size_t size)
    {
        add_simple_metric(payload.get(), name, hasAlias, alias, type, false, false, value, size);
        return *this;
    }

public:
    PayloadBuilder &seq(uint64_t value)
    {
        payload->has_seq = true;
        payload->seq = value;
        return *this;
    }

    PayloadBuilder &bdSeq(uint64_t value)
    {
        return add("bdSeq", false, 0, METRIC_DATA_TYPE_UINT64, &value, sizeof(value));
    }

    PayloadBuilder &doubleMetric(const char *name, double value, bool hasAlias = false, uint64_t alias = 0)
    {
        return add(name, hasAlias, alias, METRIC_DATA_TYPE_DOUBLE, &value, sizeof(value));
    }

    PayloadBuilder &int32Metric(const char *name, int32_t value)
    {
        return add(name, false, 0, METRIC_DATA_TYPE_INT32, &value, sizeof(value));
    }

    PayloadBuilder &stringMetric(const char *name, const char *value)
    {
        return add(name, false, 0, METRIC_DATA_TYPE_STRING, value, value ? strlen(value) : 0);
    }

    /** A metric with is_null set */
    PayloadBuilder &nullMetric(const char *name, uint32_t type)
    {
        return add(name, false, 0, type, nullptr, 0);
    }

    /** A data metric identified only by its alias, as allowed after a birth defines it */
    PayloadBuilder &aliasOnly(uint64_t alias, double value)
    {
        return add(nullptr, true, alias, METRIC_DATA_TYPE_DOUBLE, &value, sizeof(value));
    }

    /** A data metric without a datatype, as allowed in NDATA/DDATA */
    PayloadBuilder &untypedDouble(const char *name, double value)
    {
        add(name, false, 0, METRIC_DATA_TYPE_DOUBLE, &value, sizeof(value));
        auto &metric = payload->metrics[payload->metrics_count - 1];
        metric.has_datatype = false;
        metric.datatype = 0;
        return *this;
    }

    /** Adds a string property with a null value to the last metric */
    PayloadBuilder &nullStringProperty(const char *key)
    {
        auto &metric = payload->metrics[payload->metrics_count - 1];
        metric.has_properties = true;
        add_property_to_set(&metric.properties, key, PROPERTY_DATA_TYPE_STRING, nullptr, 0);
        return *this;
    }

    tahu::PayloadPtr build()
    {
        return std::move(payload);
    }
};

inline SparkplugTopic topicFor(const std::string &topic)
{
    SparkplugTopic parsed;
    parsed.parse(topic);
    return parsed;
}

/** Feeds one message into a group, the same way SparkplugHost does */
inline ParseResult send(Group &group, const std::string &topic, PayloadBuilder &builder)
{
    auto payload = builder.build();
    return group.process(topicFor(topic), payload.get());
}

inline std::vector<PublishableUpdate> poll(Group &group, bool force = false)
{
    std::vector<PublishableUpdate> updates;
    group.appendTo(updates, force);
    return updates;
}

inline const tahu::Metric *findMetric(const PublishableUpdate &update, const char *name)
{
    if (!update.payload)
    {
        return nullptr;
    }
    for (size_t i = 0; i < update.payload->metrics_count; i++)
    {
        const tahu::Metric *metric = &update.payload->metrics[i];
        if (metric->name && strcmp(metric->name, name) == 0)
        {
            return metric;
        }
    }
    return nullptr;
}

#endif /* TESTS_PAYLOADS */
