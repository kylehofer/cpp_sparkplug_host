#include "Payloads.h"
#include "Test.h"
#include "types/Node.h"
#include <chrono>

namespace
{
    const char *const NBIRTH = "spBv1.0/G/NBIRTH/N";
    const char *const NDATA = "spBv1.0/G/NDATA/N";
    const char *const NDEATH = "spBv1.0/G/NDEATH/N";
    const char *const DBIRTH = "spBv1.0/G/DBIRTH/N/D";
    const char *const DDATA = "spBv1.0/G/DDATA/N/D";

    void birth(Group &group, uint64_t bdSeq = 0)
    {
        CHECK(send(group, NBIRTH, PayloadBuilder().seq(0).bdSeq(bdSeq).doubleMetric("Temperature", 20.0, true, 7)) == ParseResult::OK);
    }

    struct RetryInterval
    {
        explicit RetryInterval(std::chrono::milliseconds interval) { Node::setRebirthRetryInterval(interval); }
        ~RetryInterval() { Node::setRebirthRetryInterval(std::chrono::seconds(5)); }
    };
}

TEST(birth_then_data_reports_birth_then_changes)
{
    Group group("G");
    birth(group);

    auto updates = poll(group);
    CHECK_EQ(updates.size(), size_t(1));
    CHECK(updates[0].type == UpdateType::BIRTH);
    CHECK_EQ(updates[0].id, std::string("G/N"));
    CHECK(findMetric(updates[0], "Temperature") != nullptr);
    CHECK(findMetric(updates[0], "bdSeq") != nullptr);

    CHECK(send(group, NDATA, PayloadBuilder().seq(1).doubleMetric("Temperature", 21.5)) == ParseResult::OK);
    updates = poll(group);
    CHECK_EQ(updates.size(), size_t(1));
    CHECK(updates[0].type == UpdateType::PUBLISH);
    CHECK_EQ(updates[0].payload->metrics_count, size_t(1));
    CHECK_EQ(findMetric(updates[0], "Temperature")->value.double_value, 21.5);

    CHECK(poll(group).empty());
}

TEST(death_before_poll_is_not_lost)
{
    Group group("G");
    birth(group);
    CHECK(send(group, NDEATH, PayloadBuilder().bdSeq(0)) == ParseResult::OK);

    auto updates = poll(group);
    CHECK_EQ(updates.size(), size_t(1));
    CHECK(updates[0].type == UpdateType::DEATH);
    CHECK(!updates[0].payload);
}

TEST(node_death_takes_devices_offline)
{
    Group group("G");
    birth(group);
    CHECK(send(group, DBIRTH, PayloadBuilder().seq(1).int32Metric("Speed", 10)) == ParseResult::OK);
    CHECK_EQ(poll(group).size(), size_t(2));

    CHECK(send(group, NDEATH, PayloadBuilder().bdSeq(0)) == ParseResult::OK);
    auto updates = poll(group);
    CHECK_EQ(updates.size(), size_t(2));
    CHECK(updates[0].type == UpdateType::DEATH);
    CHECK(updates[1].type == UpdateType::DEATH);
    CHECK_EQ(updates[1].id, std::string("G/N/D"));
}

TEST(death_from_earlier_session_is_ignored)
{
    Group group("G");
    birth(group, 3);
    poll(group);

    CHECK(send(group, NDEATH, PayloadBuilder().bdSeq(2)) == ParseResult::OK);
    CHECK(poll(group).empty());
    CHECK(group.find("G/N")->isOnline());

    CHECK(send(group, NDEATH, PayloadBuilder().bdSeq(3)) == ParseResult::OK);
    CHECK(!group.find("G/N")->isOnline());
}

TEST(sequence_mismatch_requests_one_rebirth_then_resyncs)
{
    Group group("G");
    birth(group);

    // seq 1 lost; 2 arrives
    CHECK(send(group, NDATA, PayloadBuilder().seq(2).doubleMetric("Temperature", 1)) == ParseResult::OUT_OF_SYNC);
    // Resynchronised: 3 is accepted without another rebirth
    CHECK(send(group, NDATA, PayloadBuilder().seq(3).doubleMetric("Temperature", 2)) == ParseResult::OK);
    // Another gap inside the retry interval does not storm the node
    CHECK(send(group, NDATA, PayloadBuilder().seq(9).doubleMetric("Temperature", 3)) == ParseResult::OK);
}

TEST(rebirth_retried_after_interval)
{
    RetryInterval interval(std::chrono::milliseconds(0));
    Group group("G");
    CHECK(send(group, NDATA, PayloadBuilder().seq(5).doubleMetric("Temperature", 1)) == ParseResult::OUT_OF_SYNC);
    CHECK(send(group, NDATA, PayloadBuilder().seq(6).doubleMetric("Temperature", 1)) == ParseResult::OUT_OF_SYNC);
}

TEST(data_from_unknown_node_requests_rebirth_once)
{
    Group group("G");
    CHECK(send(group, NDATA, PayloadBuilder().seq(5).doubleMetric("Temperature", 1)) == ParseResult::OUT_OF_SYNC);
    CHECK(send(group, NDATA, PayloadBuilder().seq(6).doubleMetric("Temperature", 1)) == ParseResult::OK);
    CHECK(poll(group).empty());

    birth(group);
    CHECK(poll(group)[0].type == UpdateType::BIRTH);
}

TEST(sequence_wraps_from_255_to_0)
{
    Group group("G");
    CHECK(send(group, NBIRTH, PayloadBuilder().seq(254).bdSeq(0).doubleMetric("Temperature", 1)) == ParseResult::OK);
    CHECK(send(group, NDATA, PayloadBuilder().seq(255).doubleMetric("Temperature", 2)) == ParseResult::OK);
    CHECK(send(group, NDATA, PayloadBuilder().seq(0).doubleMetric("Temperature", 3)) == ParseResult::OK);
}

TEST(data_for_unknown_device_requests_rebirth)
{
    Group group("G");
    birth(group);
    CHECK(send(group, DDATA, PayloadBuilder().seq(1).int32Metric("Speed", 1)) == ParseResult::OUT_OF_SYNC);
}

TEST(force_snapshot_does_not_consume_changes)
{
    Group group("G");
    birth(group);
    poll(group);
    CHECK(send(group, NDATA, PayloadBuilder().seq(1).doubleMetric("Temperature", 30)) == ParseResult::OK);

    auto snapshot = poll(group, true);
    CHECK_EQ(snapshot.size(), size_t(1));
    CHECK(snapshot[0].type == UpdateType::BIRTH);
    CHECK_EQ(snapshot[0].payload->metrics_count, size_t(2));

    auto updates = poll(group);
    CHECK_EQ(updates.size(), size_t(1));
    CHECK(updates[0].type == UpdateType::PUBLISH);
}
