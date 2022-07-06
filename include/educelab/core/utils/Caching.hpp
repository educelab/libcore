#pragma once

/** @file */

#include <any>
#include <cassert>
#include <cstddef>
#include <list>
#include <random>
#include <unordered_map>
#include <vector>

namespace educelab
{

namespace detail
{
/** @brief Function object for generating uniform, random integer cache keys */
template <typename T>
struct UniformIntegerKey {
    static_assert(std::is_integral_v<T>, "Integral type required");
    /** Generate a random integer */
    auto operator()() const -> T
    {
        std::random_device source;
        static std::mt19937 gen(source());
        static std::uniform_int_distribution<T> dist;
        return dist(gen);
    }
};

/**
 * @brief Least-Recently Used (LRU) caching policy
 *
 * Keeps a list of cached objects such that the LRU objects are always at the
 * end of the list. Uses a key map to accelerate list rearrangement.
 */
template <typename KeyT = std::size_t, typename SizeT = std::size_t>
class LRUPolicy
{
public:
    /** Insert a new object for policy tracking */
    void insert(KeyT key, SizeT size)
    {
        assert((entryMap_.count(key) == 0) && "Key already cached by policy");
        entryList_.emplace_front(key, size);
        entryMap_[key] = entryList_.begin();
    }

    /** Touch an object in the policy, updating last access */
    void touch(KeyT key)
    {
        // Find the key's position in the list
        auto iter = entryMap_.at(key);
        // Move to the top of the list
        entryList_.splice(entryList_.begin(), entryList_, iter);
    }

    /** Remove an object from policy tracking by key */
    void erase(KeyT key)
    {
        auto iter = entryMap_.at(key);
        entryList_.erase(iter);
        entryMap_.erase(key);
    }

    /**
     * Remove objects from the policy in order to free a specific size in
     * bytes. Returns a list of the removed objects' keys for use by the
     * data cache.
     */
    auto clear(SizeT size) -> std::vector<KeyT>
    {
        // List of keys
        std::vector<KeyT> keys;
        // Total removed
        SizeT total{0};
        while (total < size) {
            // Get the last entry
            auto back = entryList_.back();
            // Add the key to the list and update the size being removed
            keys.emplace_back(back.first);
            total += back.second;
            // Remove the entry
            entryList_.pop_back();
            entryMap_.erase(back.first);
        }
        return keys;
    }

    /** Clear the policy of all tracked objects */
    void clear()
    {
        entryList_.clear();
        entryMap_.clear();
    }

private:
    /** Policy entry record */
    using EntryRecord = std::pair<KeyT, SizeT>;
    /** LRU entry list type */
    using EntryList = std::list<EntryRecord>;
    /** Entry list key lookup map type */
    using EntryMap = std::unordered_map<KeyT, typename EntryList::iterator>;
    /** LRU-ordered entry list */
    EntryList entryList_;
    /** Entry list lookup map */
    EntryMap entryMap_;
};
}  // namespace detail

/**
 * @brief Container for size-limited caching of data objects
 *
 * This container provides a size-limited object cache for use in
 * memory-constrained applications. Set the maximum capacity of the cache in
 * bytes, and the cache will automatically remove cached data when the capacity
 * is exceeded. Which items are removed is governed by the templated `PolicyT`.
 * See detail::LRUPolicy for a description of the policy interface.
 *
 * When an object is inserted into the cache, insert() returns a unique key
 * which can be used to access the stored object at a later time. When
 * attempting to retrieve a cached object, it is always a good idea to use
 * contains() to check whether the object is still cached:
 *
 * ```{.cpp}
 * // Add integer to the cache
 * ObjectCache<int> cache;
 * auto key = cache.insert(10);
 *
 * // Check that the integer is still cached before retrieval
 * if (cache.contains(key)) {
 *   auto value = cache.get(key);
 * }
 * ```
 *
 * The default ObjectCache template uses `std::any` for storing cached objects.
 * This enables the storage of (almost) arbitrary data in a single cache.
 * However, this comes with a few caveats. First, values returned by the cache
 * must be cast back to their underlying type using `std::any_cast`. This
 * container provides no facilities for tracking the underlying type stored in
 * a `std::any` object. If you do not know the type of the stored object at time
 * of retrieval, refer to the `std::any` documentation. Second,
 * insert(const T2&) uses the `sizeof` macro to calculate the storage used by
 * the inserted object. For objects which allocate memory on the heap
 * (e.g. `std::vector`), this value will be incorrect. You must manually
 * provide the effective size of the object using insert(value_type, size_type).
 *
 * @tparam T Type of cached object
 * @tparam KeyT Type of key
 * @tparam SizeT Type for tracking object size
 * @tparam PolicyT Removal policy
 * @tparam KeyFunc Function class which returns new keys of type KeyT
 */
template <
    typename T = std::any,
    typename KeyT = std::size_t,
    typename SizeT = std::size_t,
    typename PolicyT = detail::LRUPolicy<KeyT, SizeT>,
    typename KeyFunc = detail::UniformIntegerKey<KeyT>>
class ObjectCache
{
public:
    /** Object type */
    using value_type = T;
    /** Key type */
    using key_type = KeyT;
    /** Size of objects type */
    using size_type = SizeT;
    /** Object reference type */
    using reference = value_type&;
    /** Const object reference type */
    using const_reference = const value_type&;

    /**
     * @brief Cache an object with a manually determined size
     * @return Key for accessing cached object
     */
    auto insert(value_type value, size_type size) -> key_type
    {
        // Make space for new value
        if (size_ + size > capacity_) {
            clear(size_ + size - capacity_);
        }

        // Generate unique, random key
        static KeyFunc gen;
        key_type key{0};
        do {
            key = gen();
        } while (cache_.count(key) != 0);

        // Insert into cache and policy
        cache_[key] = {std::move(value), size};
        policy_.insert(key, size);
        size_ += size;

        return key;
    }

    /**
     * @brief Cache an object with an automatically determined size
     * @return Key for accessing cached object
     */
    template <typename T2>
    auto insert(const T2& value) -> key_type
    {
        return insert(value, sizeof(value));
    }

    /** @brief Return whether the object referenced by key is in the cache */
    auto contains(key_type key) const -> bool { return cache_.count(key) == 1; }

    /** @brief Retrieve an object from the cache */
    auto get(key_type key) -> reference
    {
        reference value = cache_.at(key).value;
        policy_.touch(key);
        return value;
    }

    /** @copybrief get(key_type) */
    auto get(key_type key) const -> const_reference
    {
        const_reference value = cache_.at(key).value;
        policy_.touch(key);
        return value;
    }

    /**
     * @brief Remove an object from the cache by key
     * @return The size in bytes of the removed object
     */
    auto erase(key_type key) -> size_type
    {
        if (cache_.count(key) == 0) {
            return 0;
        }

        auto nodeHandle = cache_.extract(key);
        policy_.erase(key);
        auto size = nodeHandle.mapped().size;
        size_ -= size;

        return size;
    }

    /**
     * @brief Remove an object from the cache by key
     * @return The size in bytes of the removed object
     */
    auto clear() -> size_type
    {
        auto sizeCleared = size_;
        cache_.clear();
        policy_.clear();
        size_ = 0;
        return sizeCleared;
    }

    /**
     * @brief Remove one or more objects from the cache to free a specific
     * number of bytes
     * @return The size in bytes of all removed objects
     */
    auto clear(size_type size) -> size_type
    {
        // Update policy and get the keys removed
        auto keys = policy_.clear(size);

        // Remove values from cache
        size_type cleared{0};
        for (const auto& key : keys) {
            auto nodeHandle = cache_.extract(key);
            cleared += nodeHandle.mapped().size;
        }
        size_ -= cleared;
        return cleared;
    }

    /** @brief Set the maximum capacity of the cache in bytes */
    auto set_capacity(size_type capacity) -> size_type
    {
        // Update capacity
        capacity_ = capacity;

        // Purge if we're using too much space
        size_type cleared{0};
        if (size_ > capacity) {
            cleared = clear(size_ - capacity);
        }
        return cleared;
    }

    /** @brief Get the maximum capacity of the cache in bytes */
    [[nodiscard]] auto capacity() const -> size_type { return capacity_; }

    /** @brief Get the size of all cached objects in bytes */
    [[nodiscard]] auto size() const -> size_type { return size_; }

    /** @brief Get the number of cached objects */
    [[nodiscard]] auto count() const { return cache_.size(); }

    /** @brief Return whether the cache is empty */
    [[nodiscard]] auto empty() const -> bool { return cache_.empty(); }

private:
    /** @brief ObjectCache entry */
    struct CacheEntry {
        /** Cached object */
        value_type value;
        /** Size of cached object */
        size_type size;
    };
    /** Data cache */
    std::unordered_map<key_type, CacheEntry> cache_;
    /** Removal policy */
    mutable PolicyT policy_;
    /** Current size of the cache in bytes */
    size_type size_{0};
    /** Current cache capacity in bytes */
    size_type capacity_{10000000};
};

}  // namespace educelab