#include <gtest/gtest.h>

#include <unordered_set>

#include "educelab/core/types/Image.hpp"
#include "educelab/core/utils/Caching.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

using Cache = ObjectCache<>;

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

TEST(Caching, LRUPolicy)
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