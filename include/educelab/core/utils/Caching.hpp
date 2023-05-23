#pragma once

/** @file */

#include <any>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <list>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace educelab
{

namespace detail
{
/** @brief No-op mutex */
class NoOpMutex
{
};

/** @brief No-op lock guard implementing std::mutex compatible RAII syntax */
template <typename Mutex>
struct NoopLockGuard {
    /** No-op acquire */
    explicit NoopLockGuard(Mutex& m){};
    /** No-op acquire with locking strategy */
    template <typename LockStrategy>
    NoopLockGuard(Mutex& m, LockStrategy t){};
    /** Deleted copy constructor */
    NoopLockGuard(const NoopLockGuard&) = delete;
};

/**
 * @brief Cache synchronization policy
 *
 * Determines the cache synchronization policy which controls thread-safety. The
 * synchronization policy should be configured based on both the desired thread
 * access pattern *and* the requirements of the cache's removal policy.
 *
 * @tparam CacheSizeType The type to use for storing the cache's size and
 * capacity in bytes. Usually `std::size_t` or `std::atomic<std::size_t>`.
 * @tparam MutexType The cache mutex type. Must support RAII locking syntax.
 * @tparam WriteLock Locking type for cache write operations (e.g. insert,
 * erase, clear).
 * @tparam ReadLock Locking type for complex read operations (e.g. get,
 * find).
 * @tparam TrivialLock Locking type for trivial read operations
 * (e.g. contains, count, empty).
 */
template <
    typename CacheSizeType,
    typename MutexType,
    typename WriteLock,
    typename ReadLock,
    typename TrivialLock>
struct CacheSyncPolicy {
    /** Cache size type */
    using size_type = CacheSizeType;
    /** Cache mutex type */
    using mutex_type = MutexType;
    /** Lock guard for cache modification operations */
    using write_lock = WriteLock;
    /** Lock guard for cache retrieval operations */
    using read_lock = ReadLock;
    /** Lock guard for trivial cache read operations (size, count, etc.) */
    using trivial_lock = TrivialLock;
};

/** @brief Cache synchronization policy which performs no synchronization. */
template <typename SizeType>
using NoSyncPolicy = CacheSyncPolicy<
    SizeType,
    NoOpMutex,
    NoopLockGuard<NoOpMutex>,
    NoopLockGuard<NoOpMutex>,
    NoopLockGuard<NoOpMutex>>;

/**
 * @brief Cache synchronization policy which allows shared trivial reads but is
 * otherwise exclusive.
 */
template <typename SizeType, typename MutexType = std::shared_mutex>
using ExclusiveRWSyncPolicy = CacheSyncPolicy<
    std::atomic<SizeType>,
    MutexType,
    std::lock_guard<MutexType>,
    std::lock_guard<MutexType>,
    std::shared_lock<MutexType>>;

/** @brief Function object for generating uniform, random integer cache keys */
template <typename T>
struct UniformIntegerKey {
    static_assert(std::is_integral_v<T>, "Integral type required");
    /** Generate a random integer */
    auto operator()() const -> T
    {
        static std::mt19937 gen{std::random_device{}()};
        static std::uniform_int_distribution<T> dist;
        return dist(gen);
    }
};

/**
 * @brief Least-Recently Used (LRU) cache eviction policy
 *
 * Keeps a list of cached objects such that the LRU objects are always at the
 * end of the list. Uses a key map to accelerate list rearrangement.
 *
 * @warning This eviction policy is not thread-safe. Caches which use this
 * eviction policy in a multi-threaded context should acquire an exclusive
 * mutex lock before interacting with this policy.
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
 * bytes, and the cache will automatically evict cached data when the capacity
 * is exceeded. Which items are evicted is governed by the templated
 * `EvictionPolicy`. See detail::LRUPolicy for a description of the eviction
 * policy interface.
 *
 * When an object is inserted into the cache, insert() returns a unique key
 * which can be used to access the stored object at a later time. When
 * attempting to retrieve a cached object, it is always a good idea to use
 * contains() to check whether the object is still cached. In multi-threaded
 * applications, instead use find() to look-up and retrieve the cached object
 * in a single synchronization call:
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
 *
 * // Find and return the value in a single call
 * auto result = cache.find(key);
 * if(result) {
 *   auto value = result.value();
 * } else {
 *   std::cout << "Object not found" << std::endl;
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
 * insert(const T2&) uses the `sizeof` operator to calculate the storage used by
 * the inserted object. For objects which allocate memory on the heap
 * (e.g. `std::vector`), this value will be incorrect. You must manually
 * provide the effective size of the object using insert(value_type, size_type).
 *
 * The cache's thread-safety properties are determined by the templated
 * synchronization policy, `SyncPolicy`. This class defines the cache's mutex
 * and locking policies for three categories of data access: write, read, and
 * trivial. Write access includes all explicit cache modification calls, such
 * as insert() and erase(). Read access includes all cache read operations that
 * retrieve data from the cache, such as get() and find(). Note that read
 * operations may or may not be thread-safe depending on the eviction policy.
 * Trivial access includes all cache access which is guaranteed to not modify
 * either the cache or the eviction policy, such as calls to contains() and
 * capacity().
 *
 * For single-threaded applications, it is sufficient to use ObjectCache in
 * most circumstances. For multi-threaded applications, use
 * SynchronizedObjectCache, which utilizes a simple thread-safe synchronization
 * policy, or build your own synchronization policy. See detail::CacheSyncPolicy
 * for more information on the synchronization policy interface.
 *
 * @tparam T Type of cached object.
 * @tparam KeyT Type of key.
 * @tparam SizeT Type for tracking object size.
 * @tparam EvictionPolicy Cache eviction policy.
 * @tparam SyncPolicy Cache locking policy.
 * @tparam KeyFunc Function class which returns new keys of type KeyT.
 */
template <
    typename T = std::any,
    typename KeyT = std::size_t,
    typename SizeT = std::size_t,
    typename EvictionPolicy = detail::LRUPolicy<KeyT, SizeT>,
    typename SyncPolicy = detail::NoSyncPolicy<SizeT>,
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

    /**
     * @brief Cache an object with a manually determined size
     * @return Key for accessing cached object
     */
    auto insert(value_type value, size_type size) -> key_type
    {
        // Exclusive access
        const typename sync_policy::write_lock lock(mutex_);

        // Make space for new value
        if (size_ + size > capacity_) {
            clear_(size_ + size - capacity_);
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
    auto contains(key_type key) const -> bool
    {
        const typename sync_policy::trivial_lock lock(mutex_);
        return cache_.count(key) == 1;
    }

    /** @brief Retrieve an object from the cache */
    auto get(key_type key) -> value_type
    {
        const typename sync_policy::read_lock lock(mutex_);
        auto value = cache_.at(key).value;
        policy_.touch(key);
        return value;
    }

    /** @copybrief get(key_type) */
    auto get(key_type key) const -> value_type
    {
        const typename sync_policy::read_lock lock(mutex_);
        auto value = cache_.at(key).value;
        policy_.touch(key);
        return value;
    }

    /** @brief Retrieve an object from the cache if it exists */
    auto find(key_type key) -> std::optional<value_type>
    {
        const typename sync_policy::read_lock lock(mutex_);

        std::optional<value_type> value;
        if (cache_.count(key) == 1) {
            value = cache_.at(key).value;
        } else {
            return value;
        }

        policy_.touch(key);
        return value;
    }

    /** @copybrief find(key_type) */
    auto find(key_type key) const -> std::optional<value_type>
    {
        const typename sync_policy::read_lock lock(mutex_);

        std::optional<value_type> value;
        if (cache_.count(key) == 1) {
            value = cache_.at(key).value;
        } else {
            return value;
        }

        policy_.touch(key);
        return value;
    }

    /**
     * @brief Remove an object from the cache by key
     * @return The size in bytes of the removed object
     */
    auto erase(key_type key) -> size_type
    {
        // Exclusive erase
        const typename sync_policy::write_lock lock(mutex_);

        // Check for key
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
        const typename sync_policy::write_lock lock(mutex_);
        return clear_();
    }

    /**
     * @brief Remove one or more objects from the cache to free a specific
     * number of bytes
     * @return The size in bytes of all removed objects
     */
    auto clear(size_type size) -> size_type
    {
        const typename sync_policy::write_lock lock(mutex_);
        return clear_(size);
    }

    /** @brief Set the maximum capacity of the cache in bytes */
    auto set_capacity(size_type capacity) -> size_type
    {
        // Exclusive access
        const typename sync_policy::write_lock lock(mutex_);

        // Update capacity
        capacity_ = capacity;

        // Purge if we're using too much space
        size_type cleared{0};
        if (size_ > capacity) {
            cleared = clear_(size_ - capacity);
        }
        return cleared;
    }

    /** @brief Get the maximum capacity of the cache in bytes */
    [[nodiscard]] auto capacity() const -> size_type { return capacity_; }

    /** @brief Get the size of all cached objects in bytes */
    [[nodiscard]] auto size() const -> size_type { return size_; }

    /** @brief Get the number of cached objects */
    [[nodiscard]] auto count() const
    {
        const typename sync_policy::trivial_lock lock(mutex_);
        return cache_.size();
    }

    /** @brief Return whether the cache is empty */
    [[nodiscard]] auto empty() const -> bool
    {
        const typename sync_policy::trivial_lock lock(mutex_);
        return cache_.empty();
    }

private:
    /** Private clear() implementation */
    auto clear_() -> size_type
    {
        size_type sizeCleared = size_;
        cache_.clear();
        policy_.clear();
        size_ = 0;
        return sizeCleared;
    }

    /** Private clear(size_type) implementation */
    auto clear_(size_type size) -> size_type
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

    /** @brief ObjectCache entry */
    struct CacheEntry {
        /** Cached object */
        value_type value;
        /** Size of cached object */
        size_type size;
    };

    /** Synchronization policy type */
    using sync_policy = SyncPolicy;
    /** Eviction policy type */
    using eviction_policy = EvictionPolicy;

    /** Data cache */
    std::unordered_map<key_type, CacheEntry> cache_;
    /** Eviction policy instance */
    mutable eviction_policy policy_;
    /** Cache mutex */
    mutable typename sync_policy::mutex_type mutex_;
    /** Current size of the cache in bytes */
    typename sync_policy::size_type size_{0};
    /** Current cache capacity in bytes */
    typename sync_policy::size_type capacity_{10'000'000};
};

/**
 * @brief Specialization of ObjectCache which uses an exclusive locking policy.
 *
 * @tparam T Type of cached object.
 * @tparam KeyT Type of key.
 * @tparam SizeT Type for tracking object size.
 * @tparam EvictionPolicy Cache eviction policy.
 * @tparam KeyFunc Function class which returns new keys of type KeyT.
 */
template <
    typename T = std::any,
    typename KeyT = std::size_t,
    typename SizeT = std::size_t,
    typename EvictionPolicy = detail::LRUPolicy<KeyT, SizeT>,
    typename KeyFunc = detail::UniformIntegerKey<KeyT>>
using SynchronizedObjectCache = ObjectCache<
    T,
    KeyT,
    SizeT,
    EvictionPolicy,
    detail::ExclusiveRWSyncPolicy<SizeT, std::shared_mutex>,
    KeyFunc>;

}  // namespace educelab