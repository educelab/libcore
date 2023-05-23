#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

#include "educelab/core/types/Image.hpp"
#include "educelab/core/utils/Caching.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

using Cache = ObjectCache<>;

// Simple thread pool class
// Adapted from: https://stackoverflow.com/a/32593825
// WARNING: Not tested for production code.
class ThreadPool
{
public:
    ThreadPool() = default;
    ~ThreadPool() { stop(); }

    void start()
    {
        threads_.resize(ts_);
        for (auto t : range(ts_)) {
            threads_.at(t) = std::thread(&ThreadPool::loop_, this);
        }
    }

    void push_back(const std::function<void()>& job)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        jobs_.push(job);
        lock.unlock();
        condition_.notify_one();
    }

    void stop()
    {
        terminate_ = true;
        condition_.notify_all();
        for (auto& t : threads_) {
            t.join();
        }
        threads_.clear();
    }

    auto busy() -> bool
    {
        bool isBusy{true};
        std::unique_lock<std::mutex> lock(mutex_);
        isBusy = not jobs_.empty();
        lock.unlock();
        return isBusy;
    }

private:
    void loop_()
    {
        while (not terminate_) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(
                    lock, [this] { return not jobs_.empty() or terminate_; });
                if (terminate_) {
                    return;
                }
                job = jobs_.front();
                jobs_.pop();
            }
            job();
        }
    }
    std::atomic_bool terminate_{false};
    std::mutex mutex_;
    std::condition_variable condition_;
    std::size_t ts_{std::max(1U, std::thread::hardware_concurrency() - 1)};
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> jobs_;
};

TEST(Caching, SimpleInsertErase)
{
    // Create cache
    Cache cache;

    // Insert
    auto key = cache.insert(10);
    EXPECT_TRUE(cache.contains(key));
    EXPECT_EQ(cache.count(), 1);
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), sizeof(int));

    // Get
    auto result = cache.get(key);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(std::any_cast<int>(result), 10);

    // Erase
    auto erased = cache.erase(key);
    EXPECT_FALSE(cache.contains(key));
    EXPECT_EQ(cache.count(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(erased, sizeof(int));
    EXPECT_EQ(cache.size(), 0);
}

TEST(Caching, MassInsertClear)
{
    // Create cache
    Cache cache;

    // Insert 100x values
    std::vector<Cache::key_type> keys;
    for (const auto& val : range(100)) {
        keys.emplace_back(cache.insert(val));
        EXPECT_TRUE(cache.contains(keys.back()));
    }
    EXPECT_EQ(cache.count(), 100);
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), sizeof(int) * 100);

    // Check that all values match
    for (const auto& [idx, key] : enumerate(keys)) {
        auto result = cache.get(key);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(idx, std::any_cast<int>(result));
    }

    // Clear 50%
    auto erased = cache.clear(sizeof(int) * 50);
    EXPECT_EQ(cache.count(), 50);
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(erased, sizeof(int) * 50);
    EXPECT_EQ(cache.size(), sizeof(int) * 50);

    // Clear the rest
    cache.clear();
    EXPECT_EQ(cache.count(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0);
}

TEST(Caching, UniqueKeys)
{
    // Create cache
    Cache cache;

    // Insert 100x values
    std::vector<Cache::key_type> keys;
    for (auto val : range(100)) {
        keys.emplace_back(cache.insert(val));
    }

    // Remove duplicates with unordered_set
    std::unordered_set<Cache::key_type> set(keys.begin(), keys.end());
    EXPECT_EQ(set.size(), keys.size());
}

TEST(Caching, HeterogeneousData)
{
    // Create cache
    Cache cache;

    // Insert int
    auto k1 = cache.insert(10);
    auto expectedSize = sizeof(int);

    // Insert vector of ints, manually set size
    std::vector<int> listIn{0, 1, 2, 3, 4};
    auto k2 = cache.insert(listIn, sizeof(int) * listIn.size());
    expectedSize += sizeof(int) * listIn.size();

    // Insert image
    Image imgIn(200, 100, 3, Depth::U8);
    auto k3 = cache.insert(imgIn, imgIn.size());
    expectedSize += imgIn.size();

    // Test insertion
    EXPECT_TRUE(cache.contains(k1));
    EXPECT_TRUE(cache.contains(k2));
    EXPECT_TRUE(cache.contains(k3));
    EXPECT_EQ(cache.count(), 3);
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), expectedSize);

    // Test stored int
    EXPECT_EQ(std::any_cast<int>(cache.get(k1)), 10);
    // Test stored list
    auto listOut = std::any_cast<std::vector<int>>(cache.get(k2));
    for (const auto& [idx, val] : enumerate(listOut)) {
        EXPECT_EQ(val, listIn.at(idx));
    }
    // Test stored image
    auto imgOut = std::any_cast<Image>(cache.get(k3));
    EXPECT_TRUE(imgOut);
    EXPECT_EQ(imgOut.height(), imgIn.height());
    EXPECT_EQ(imgOut.width(), imgIn.width());
    EXPECT_EQ(imgOut.channels(), imgIn.channels());
    EXPECT_EQ(imgOut.type(), imgIn.type());
}

TEST(Caching, LRUCache)
{
    // Create cache
    Cache cache;

    // Limit capacity
    cache.set_capacity(sizeof(int) * 50);
    EXPECT_EQ(cache.capacity(), sizeof(int) * 50);

    // Insert 100x values
    std::vector<Cache::key_type> keys;
    for (const auto& val : range(100)) {
        keys.emplace_back(cache.insert(val));
    }

    // Check that the size is limited to capacity
    EXPECT_EQ(cache.size(), cache.capacity());
    EXPECT_EQ(cache.count(), 50);

    // Check that only the last 50 values are in the cache
    for (const auto& [idx, key] : enumerate(keys)) {
        if (idx < 50) {
            EXPECT_FALSE(cache.contains(key));
        } else {
            EXPECT_TRUE(cache.contains(key));
        }
    }

    // Touch the oldest key still in the cache
    auto keepAlive = keys.at(50);
    cache.get(keepAlive);

    // Insert a new value
    auto newKey = cache.insert(10);

    // Check what was removed
    auto expectRemoved = keys.at(51);
    EXPECT_TRUE(cache.contains(keepAlive));
    EXPECT_TRUE(cache.contains(newKey));
    EXPECT_FALSE(cache.contains(expectRemoved));
}

TEST(Caching, SpecializedCache)
{
    // Create int-only cache
    ObjectCache<int> cache;

    // Insert
    auto key = cache.insert(10);
    EXPECT_TRUE(cache.contains(key));
    EXPECT_EQ(cache.count(), 1);
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), sizeof(int));

    // Get
    auto result = cache.get(key);
    EXPECT_EQ(result, 10);

    // Erase
    auto erased = cache.erase(key);
    EXPECT_FALSE(cache.contains(key));
    EXPECT_EQ(cache.count(), 0);
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(erased, sizeof(int));
    EXPECT_EQ(cache.size(), 0);
}

TEST(Caching, EmptyAccess)
{
    Cache cache;
    EXPECT_FALSE(cache.contains(0));
    EXPECT_THROW(auto a = cache.get(0), std::out_of_range);
    EXPECT_THROW(const auto b = cache.get(0), std::out_of_range);
    EXPECT_NO_THROW(cache.erase(0));
    EXPECT_FALSE(cache.find(0));
}

TEST(Caching, DefaultCopy)
{
    // Create first cache
    ObjectCache<int> a;
    auto key0 = a.insert(0);

    // Copy construct
    ObjectCache<int> b(a);
    auto key1 = b.insert(1);

    // Test the sizes
    EXPECT_EQ(b.count(), 2);
    EXPECT_NE(a.count(), b.count());
    EXPECT_EQ(b.get(key0), a.get(key0));
    EXPECT_EQ(b.get(key1), 1);

    // Copy operator
    ObjectCache<int> c;
    c = b;
    auto key2 = c.insert(2);

    // Test the sizes
    EXPECT_EQ(c.count(), 3);
    EXPECT_NE(b.count(), c.count());
    EXPECT_EQ(c.get(key0), a.get(key0));
    EXPECT_EQ(c.get(key1), b.get(key1));
    EXPECT_EQ(c.get(key2), 2);
}

TEST(Caching, DefaultMove)
{
    // Create first cache
    ObjectCache<int> a;
    auto key0 = a.insert(0);

    // Move construct
    ObjectCache<int> b(std::move(a));
    auto key1 = b.insert(1);

    // Test the sizes
    EXPECT_EQ(b.count(), 2);
    EXPECT_EQ(b.get(key0), 0);
    EXPECT_EQ(b.get(key1), 1);

    // Move operator
    ObjectCache<int> c;
    c = std::move(b);
    auto key2 = c.insert(2);

    // Test the sizes
    EXPECT_EQ(c.count(), 3);
    EXPECT_EQ(c.get(key0), 0);
    EXPECT_EQ(c.get(key1), 1);
    EXPECT_EQ(c.get(key2), 2);
}

TEST(Caching, SharedPtr)
{
    // Create cache
    Cache cache;

    // Create key and weak_ptr to object
    Cache::key_type key{0};
    std::weak_ptr<int> weak;

    // Create scoped, shared int and put in the cache
    {
        auto shared = std::make_shared<int>(10);
        weak = shared;
        key = cache.insert(shared, sizeof(int));
    }

    // Check access through the weak_ptr
    {
        EXPECT_TRUE(weak.use_count() != 0);
        auto shared = weak.lock();
        EXPECT_TRUE(shared);
        EXPECT_EQ(*shared, 10);
    }

    // Check access through the cache
    {
        auto shared = std::any_cast<std::shared_ptr<int>>(cache.get(key));
        EXPECT_TRUE(shared);
        EXPECT_EQ(*shared, 10);
    }

    // Delete cache reference
    cache.erase(key);

    // Check no access through the weak_ptr
    {
        EXPECT_TRUE(weak.use_count() == 0);
        auto shared = weak.lock();
        EXPECT_FALSE(shared);
    }
}

TEST(Caching, Threading)
{
    // Setup cache
    using IntCache = SynchronizedObjectCache<int>;
    IntCache cache;
    cache.set_capacity(sizeof(int) * 5'000);

    // Add some keys to the cache
    std::vector<IntCache::key_type> keys;
    for (auto i : range(5'000)) {
        keys.emplace_back(cache.insert(i));
    }

    // Multi-thread insertions and retrievals
    ThreadPool pool;
    std::atomic<std::size_t> hits{0};
    for (auto i : range(5'000)) {
        // Try to retrieve one of our starting keys
        pool.push_back([&cache, &keys, &hits, i]() {
            auto value = cache.find(keys[i]);
            if (value) {
                hits++;
            }
        });
        // Insert new key
        pool.push_back([&cache, i]() { cache.insert(i); });
    }

    // Run the thread pool
    pool.start();
    while (pool.busy()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Confirm that whatever is in the cache is what we expect
    for (const auto [expected, key] : enumerate(keys)) {
        auto val = cache.find(key);
        if (val) {
            EXPECT_EQ(val.value(), expected);
        }
    }
}