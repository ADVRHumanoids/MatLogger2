#include <matlogger2/utils/spsc_queue.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#define EIGEN_RUNTIME_NO_MALLOC
#include <Eigen/Core>

namespace {
struct Tracked
{
    static int constructions;
    static int destructions;
    int value = 0;
    Tracked() { ++constructions; }
    Tracked(const Tracked& other) : value(other.value) { ++constructions; }
    ~Tracked() { ++destructions; }
};
int Tracked::constructions = 0;
int Tracked::destructions = 0;
}

TEST(SpscQueue, PersistentSlotsAndWraparound)
{
    Tracked::constructions = Tracked::destructions = 0;
    {
        XBot::detail::SpscQueue<Tracked, 2> queue;
        EXPECT_EQ(Tracked::constructions, 3); // includes the sentinel slot
        Tracked input, output;
        const auto constructions = Tracked::constructions;
        EXPECT_FALSE(queue.pop(output));
        for(int round = 0; round < 100; ++round)
        {
            input.value = round * 2;
            EXPECT_TRUE(queue.push(input));
            ++input.value;
            EXPECT_TRUE(queue.push(input));
            EXPECT_EQ(queue.write_available(), 0u);
            EXPECT_FALSE(queue.push(input));
            EXPECT_TRUE(queue.pop(output));
            EXPECT_EQ(output.value, round * 2);
            EXPECT_EQ(queue.consume_all([&](const Tracked& value) {
                EXPECT_EQ(value.value, round * 2 + 1);
            }), 1u);
            EXPECT_EQ(queue.write_available(), 2u);
        }
        EXPECT_EQ(Tracked::constructions, constructions);
        EXPECT_EQ(Tracked::destructions, 0);
    }
    EXPECT_EQ(Tracked::constructions, Tracked::destructions);
}

TEST(SpscQueue, RetainsConsumedSharedPointerUntilOverwrite)
{
    std::weak_ptr<int> retained;
    {
        XBot::detail::SpscQueue<std::shared_ptr<int>, 1> queue;
        auto value = std::make_shared<int>(42);
        retained = value;
        ASSERT_TRUE(queue.push(value));
        value.reset();
        std::shared_ptr<int> output;
        ASSERT_TRUE(queue.pop(output));
        output.reset();
        EXPECT_FALSE(retained.expired());
        // Wrap through the sentinel and replace the original slot.
        ASSERT_TRUE(queue.push({}));
        ASSERT_TRUE(queue.pop(output));
        ASSERT_TRUE(queue.push({}));
        EXPECT_TRUE(retained.expired());
    }
}

TEST(SpscQueue, ConcurrentFifo)
{
    XBot::detail::SpscQueue<int, 20> queue;
    constexpr int count = 1000000;
    std::thread producer([&] {
        for(int value = 0; value < count; ++value)
            while(!queue.push(value)) {}
    });
    int mismatches = 0;
    for(int expected = 0; expected < count; ++expected)
    {
        int value;
        while(!queue.pop(value)) {}
        if(value != expected) ++mismatches;
    }
    producer.join();
    EXPECT_EQ(mismatches, 0);
    EXPECT_EQ(queue.write_available(), 20u);
}

TEST(SpscQueue, ResetPreallocatesDynamicPayloadsIncludingSentinel)
{
    XBot::detail::SpscQueue<Eigen::VectorXd, 2> queue;
    Eigen::VectorXd value = Eigen::VectorXd::Ones(100);
    Eigen::VectorXd output = Eigen::VectorXd::Zero(100);
    queue.reset(value);
    Eigen::internal::set_is_malloc_allowed(false);
    for(int i = 0; i < 100; ++i)
    {
        EXPECT_TRUE(queue.push(value));
        EXPECT_TRUE(queue.pop(output));
    }
    Eigen::internal::set_is_malloc_allowed(true);
    EXPECT_EQ(output, value);
    EXPECT_TRUE(queue.push(value));
    queue.reset(value);
    EXPECT_FALSE(queue.pop(output));
    EXPECT_EQ(queue.write_available(), 2u);
}
