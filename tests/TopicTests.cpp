#include "Test.h"
#include "utilities/HostState.h"
#include "utilities/SparkplugTopic.h"

TEST(topic_parses_node_message)
{
    SparkplugTopic topic;
    CHECK(topic.parse("spBv1.0/Plant1/NDATA/Edge1"));
    CHECK_EQ(topic.getGroup(), std::string("Plant1"));
    CHECK_EQ(topic.getNode(), std::string("Edge1"));
    CHECK(!topic.isDevice());
    CHECK(topic.isData());
}

TEST(topic_parses_device_message)
{
    SparkplugTopic topic;
    CHECK(topic.parse("spBv1.0/Plant1/DBIRTH/Edge1/Pump"));
    CHECK(topic.isDevice());
    CHECK(topic.isBirth());
    CHECK_EQ(topic.getDevice(), std::string("Pump"));
}

TEST(topic_rejects_invalid_topics)
{
    SparkplugTopic topic;
    CHECK(!topic.parse("spBv1.0/STATE/host"));
    CHECK(!topic.parse("spAv1.0/Plant1/NDATA/Edge1"));
    CHECK(!topic.parse("spBv1.0/Plant1/NDATA"));
    CHECK(!topic.parse("spBv1.0/Plant1/NDATA/Edge1/Pump"));
    CHECK(!topic.parse("spBv1.0/Plant1/DDATA/Edge1"));
    CHECK(!topic.parse("spBv1.0//NDATA/Edge1"));
    CHECK(!topic.parse("spBv1.0/Plant1/NDATA/Edge1/"));
    CHECK(!topic.parse("spBv1.0/Plant1/BOGUS/Edge1"));
}

TEST(topic_reparse_clears_device)
{
    SparkplugTopic topic;
    CHECK(topic.parse("spBv1.0/G/DDATA/N/D"));
    CHECK(topic.parse("spBv1.0/G/NDATA/N"));
    CHECK(!topic.isDevice());
    CHECK(topic.getDevice().empty());
}

TEST(host_state_parses_spaced_and_compact_json)
{
    auto spaced = HostState::parse("{ \"online\": false, \"timestamp\": 1700000000000 }");
    CHECK(spaced.has_value());
    CHECK(!spaced->online);
    CHECK_EQ(spaced->timestamp, 1700000000000ULL);

    auto compact = HostState::parse("{\"timestamp\":5,\"online\":true}");
    CHECK(compact.has_value());
    CHECK(compact->online);
    CHECK_EQ(compact->timestamp, 5ULL);
}

TEST(host_state_rejects_invalid_json)
{
    CHECK(!HostState::parse("{}").has_value());
    CHECK(!HostState::parse("{\"online\": 1}").has_value());
    CHECK(!HostState::parse("not json").has_value());
}

TEST(host_state_round_trips)
{
    HostState state{true, 42};
    auto parsed = HostState::parse(state.toJson());
    CHECK(parsed.has_value());
    CHECK(parsed->online);
    CHECK_EQ(parsed->timestamp, 42ULL);
}
