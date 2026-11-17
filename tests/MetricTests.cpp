#include "Payloads.h"
#include "Test.h"
#include <string>

namespace
{
    const char *const NBIRTH = "spBv1.0/G/NBIRTH/N";
    const char *const NDATA = "spBv1.0/G/NDATA/N";
}

TEST(alias_only_data_updates_metric)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0, true, 42)) == ParseResult::OK);
    poll(group);

    CHECK(send(group, NDATA, PayloadBuilder().seq(1).aliasOnly(42, 2.5)) == ParseResult::OK);
    auto updates = poll(group);
    CHECK_EQ(updates.size(), size_t(1));
    CHECK_EQ(findMetric(updates[0], "Pressure")->value.double_value, 2.5);
}

TEST(unknown_alias_requests_rebirth)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0, true, 42)) == ParseResult::OK);
    CHECK(send(group, NDATA, PayloadBuilder().seq(1).aliasOnly(99, 2.5)) == ParseResult::OUT_OF_SYNC);
}

TEST(null_string_metric_is_reported_as_null)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).nullMetric("Label", METRIC_DATA_TYPE_STRING)) == ParseResult::OK);

    auto updates = poll(group);
    const tahu::Metric *label = findMetric(updates[0], "Label");
    CHECK(label != nullptr);
    CHECK(label->has_is_null && label->is_null);
    CHECK_EQ(label->datatype, uint32_t(METRIC_DATA_TYPE_STRING));
}

TEST(string_metric_round_trips)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).stringMetric("Label", "hello")) == ParseResult::OK);
    auto updates = poll(group);
    CHECK_EQ(std::string(findMetric(updates[0], "Label")->value.string_value), std::string("hello"));
}

TEST(null_string_property_does_not_crash)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0).nullStringProperty("Units")) == ParseResult::OK);
    auto updates = poll(group);
    const tahu::Metric *pressure = findMetric(updates[0], "Pressure");
    CHECK(pressure->has_properties);
    CHECK_EQ(pressure->properties.keys_count, size_t(1));
    CHECK(pressure->properties.values[0].is_null);
}

TEST(data_without_datatype_uses_birth_type)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0)) == ParseResult::OK);
    poll(group);
    CHECK(send(group, NDATA, PayloadBuilder().seq(1).untypedDouble("Pressure", 3.0)) == ParseResult::OK);
    CHECK_EQ(findMetric(poll(group)[0], "Pressure")->value.double_value, 3.0);
}

TEST(datatype_change_requests_rebirth)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0)) == ParseResult::OK);
    CHECK(send(group, NDATA, PayloadBuilder().seq(1).int32Metric("Pressure", 3)) == ParseResult::OUT_OF_SYNC);
}

TEST(metric_not_in_birth_requests_rebirth_but_keeps_others)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).doubleMetric("Pressure", 1.0)) == ParseResult::OK);
    poll(group);
    CHECK(send(group, NDATA, PayloadBuilder().seq(1).doubleMetric("Unknown", 1.0).doubleMetric("Pressure", 4.0)) == ParseResult::OUT_OF_SYNC);
    CHECK_EQ(findMetric(poll(group)[0], "Pressure")->value.double_value, 4.0);
}

TEST(int32_value_preserved)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(0).int32Metric("Count", -5)) == ParseResult::OK);
    auto updates = poll(group);
    const tahu::Metric *count = findMetric(updates[0], "Count");
    CHECK_EQ(static_cast<int32_t>(count->value.int_value), -5);
}

TEST(large_payload_encodes_and_decodes)
{
    PayloadBuilder builder;
    std::string big(10000, 'x');
    builder.stringMetric("Big", big.c_str());
    auto payload = builder.build();

    auto encoded = tahu::encode(payload.get());
    CHECK(encoded.has_value());
    CHECK(encoded->size() > 10000);

    auto decoded = tahu::decode(encoded->data(), encoded->size());
    CHECK(decoded != nullptr);
    CHECK_EQ(std::string(decoded->metrics[0].value.string_value).size(), size_t(10000));
}

TEST(invalid_bytes_fail_to_decode)
{
    const uint8_t garbage[] = {0xff, 0xff, 0xff, 0xff};
    CHECK(tahu::decode(garbage, sizeof(garbage)) == nullptr);
}
