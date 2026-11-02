#ifndef MATLOGGER2_SPSC_QUEUE_H
#define MATLOGGER2_SPSC_QUEUE_H

#include <array>
#include <atomic>
#include <cstddef>

namespace XBot { namespace detail {

// Fixed-capacity SPSC queue with persistent, preconstructed elements. Unlike
// boost::lockfree::spsc_queue, consuming an element does not destroy its slot.
// Each index has exactly one writer; release/acquire publishes slot accesses.
template <typename T, std::size_t Capacity>
class SpscQueue
{
    static_assert(Capacity > 0, "queue capacity must be positive");
    static constexpr std::size_t StorageSize = Capacity + 1;

    static std::size_t next(std::size_t index)
    {
        return index == Capacity ? 0 : index + 1;
    }

public:
    // Not thread-safe: call before starting producer/consumer threads. Assign
    // every slot, including the sentinel, to preallocate dynamic payloads.
    void reset(const T& value)
    {
        for(auto& slot : _storage) slot = value;
        _read.store(0, std::memory_order_relaxed);
        _write.store(0, std::memory_order_relaxed);
    }

    bool push(const T& value)
    {
        const auto write = _write.load(std::memory_order_relaxed);
        const auto following = next(write);
        if(following == _read.load(std::memory_order_acquire))
            return false;
        _storage[write] = value;
        _write.store(following, std::memory_order_release);
        return true;
    }

    bool pop(T& value)
    {
        return consume_one([&value](const T& slot) { value = slot; });
    }

    template <typename Functor>
    std::size_t consume_all(Functor functor)
    {
        auto read = _read.load(std::memory_order_relaxed);
        const auto write = _write.load(std::memory_order_acquire);
        std::size_t count = 0;
        while(read != write)
        {
            functor(_storage[read]);
            read = next(read);
            ++count;
        }
        _read.store(read, std::memory_order_release);
        return count;
    }

    // Producer-side capacity snapshot, as used by VariableBuffer callbacks.
    std::size_t write_available() const
    {
        const auto write = _write.load(std::memory_order_relaxed);
        const auto read = _read.load(std::memory_order_acquire);
        return read > write ? read - write - 1 : Capacity - write + read;
    }

private:
    template <typename Functor>
    bool consume_one(Functor functor)
    {
        const auto read = _read.load(std::memory_order_relaxed);
        if(read == _write.load(std::memory_order_acquire))
            return false;
        functor(_storage[read]);
        _read.store(next(read), std::memory_order_release);
        return true;
    }

    // Separate producer and consumer indices to avoid false sharing.
    // Padding rather than over-alignment also supports C++14 heap allocation.
    std::atomic<std::size_t> _write{0};
    char _padding[64 - sizeof(std::atomic<std::size_t>)];
    std::atomic<std::size_t> _read{0};
    std::array<T, StorageSize> _storage{};
};

}} // namespace XBot::detail

#endif
