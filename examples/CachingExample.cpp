#include <iostream>
#include <vector>

#include "educelab/core/utils/Caching.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

// Key type
using Key = std::size_t;

// Helper: Print ObjectCache<int>
template <typename KeyList, typename Cache>
void print_int_cache(const KeyList& keys, const Cache& cache)
{
    std::cout << "Cached: ";
    for (const auto& key : keys) {
        if (cache.contains(key)) {
            std::cout << cache.get(key) << " ";
        }
    }
    std::cout << "\n";
}

// Helper: Print ObjectCache<>
template <typename KeyList, typename Cache>
void print_cache(const KeyList& keys, const Cache& cache)
{
    std::cout << "Cached: ";
    for (const auto& key : keys) {
        // Skip keys no longer in the cache
        if (not cache.contains(key)) {
            continue;
        }
        // Get the stored value
        auto val = cache.get(key);

        // Handle int
        if (val.type() == typeid(int)) {
            std::cout << std::any_cast<int>(val) << " ";
        }
        // Hande list of int
        else if (val.type() == typeid(std::vector<int>)) {
            for (const auto& v : std::any_cast<std::vector<int>>(val)) {
                std::cout << std::any_cast<int>(v) << " ";
            }
        }
    }
    std::cout << "\n";
}

auto main() -> int
{
    // Construct a specialized cache
    std::cout << "--- Int Cache ---\n";
    ObjectCache<int> intCache;

    // Insert 10 values
    std::vector<Key> keys;
    for (const auto& val : range(10)) {
        keys.emplace_back(intCache.insert(val));
    }

    // Check that all values are cached
    print_int_cache(keys, intCache);

    // Limit size to 5 ints
    intCache.set_capacity(sizeof(int) * 5);
    print_int_cache(keys, intCache);
    std::cout << "\n";

    // Construct a generic cache
    std::cout << "--- Generic Cache ---\n";
    keys.clear();
    ObjectCache cache;

    // Store 5 ints
    for (auto val : range(5)) {
        keys.emplace_back(cache.insert(val));
    }

    // Store a vector of ints
    const std::vector<int> vals{5, 6, 7, 8, 9};
    keys.emplace_back(cache.insert(vals, sizeof(int) * vals.size()));

    // Print all cached values
    print_cache(keys, cache);

    // Limit size to 6 ints
    cache.set_capacity(sizeof(int) * 6);
    print_cache(keys, cache);

    // Limit size to 3 ints
    cache.set_capacity(sizeof(int) * 3);
    print_cache(keys, cache);
}
