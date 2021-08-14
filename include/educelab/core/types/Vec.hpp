#pragma once

/** @file */

#include <array>
#include <iostream>

#include "educelab/core/utils/Math.hpp"

namespace educelab
{

/**
 * @brief N-dimensional vector class
 *
 * Essentially a wrapper around std::array that makes it more convenient for
 * vector math purposes.
 *
 * @tparam T Element type
 * @tparam Dims Number of elements
 */
template <
    typename T,
    std::size_t Dims,
    std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
class Vec
{
    /** Underlying element storage */
    using Container = std::array<T, Dims>;

public:
    /** Element type */
    using value_type = T;
    /** Vector size type */
    using size_type = typename Container::size_type;
    /** Difference type */
    using difference_type = typename Container::difference_type;
    /** Reference type */
    using reference = value_type&;
    /** Const reference type */
    using const_reference = const value_type&;
    /** Pointer type */
    using pointer = value_type*;
    /** Const pointer type */
    using const_pointer = const value_type*;
    /** Iterator type */
    using iterator = typename Container::iterator;
    /** Const iterator type */
    using const_iterator = typename Container::const_iterator;
    /** Reverse iterator type */
    using reverse_iterator = typename Container::reverse_iterator;
    /** Const reverse iterator type */
    using const_reverse_iterator = typename Container::const_reverse_iterator;

    /** @brief Default constructor */
    Vec() { val_.fill(0); }

    /**
     * @brief Construct with element values
     *
     * The number of arguments provided must match Dims.
     */
    template <typename... Args>
    explicit Vec(Args... args)
    {
        static_assert(sizeof...(args) == Dims, "Incorrect number of arguments");
        std::size_t i{0};
        ((val_[i++] = args), ...);
    }

    /** @brief Copy constructor */
    template <typename Vector>
    explicit Vec(const Vector& vec)
    {
        std::copy(val_.begin(), val_.end(), std::begin(vec));
    }

    /** @brief Bounds-checked element access */
    constexpr auto at(size_type pos) -> reference { return val_.at(pos); }
    /** @brief Bounds-checked element access */
    constexpr auto at(size_type pos) const -> const_reference
    {
        return val_.at(pos);
    }
    /** @brief Element access */
    constexpr auto operator[](size_type i) -> reference { return val_[i]; }
    /** @brief Element access */
    constexpr auto operator[](size_type i) const -> const_reference
    {
        return val_[i];
    }

    /** @brief First element */
    constexpr auto front() -> reference { return val_.front(); }
    /** @brief First element */
    constexpr auto front() const -> const_reference { return val_.front(); }
    /** @brief Last element */
    constexpr auto back() -> reference { return val_.back(); }
    /** @brief Last element */
    constexpr auto back() const -> const_reference { return val_.back(); }

    /** @brief Get a pointer to the first element of the raw data */
    constexpr auto data() -> pointer { return val_.data(); }
    /** @brief Get a pointer to the first element of the raw data */
    constexpr auto data() const -> const_pointer { return val_.data(); }

    /** @brief Get an iterator to the first element of the vector */
    constexpr auto begin() noexcept -> iterator { return val_.begin(); }
    /** @brief Get an iterator to the first element of the vector */
    constexpr auto begin() const noexcept -> const_iterator
    {
        return val_.begin();
    }
    /** @brief Get an iterator to the first element of the vector */
    constexpr auto cbegin() const noexcept -> const_iterator
    {
        return val_.cbegin();
    }

    /** @brief Get an iterator to one past the last element in the vector */
    constexpr auto end() noexcept -> iterator { return val_.end(); }
    /** @brief Get an iterator to one past the last element in the vector */
    constexpr auto end() const noexcept -> const_iterator { return val_.end(); }
    /** @brief Get an iterator to one past the last element in the vector */
    constexpr auto cend() const noexcept -> const_iterator
    {
        return val_.cend();
    }

    /** @brief Get an iterator to the first element of the reverse vector */
    constexpr auto rbegin() noexcept -> iterator { return val_.rbegin(); }
    /** @brief Get an iterator to the first element of the vector */
    constexpr auto rbegin() const noexcept -> const_iterator
    {
        return val_.rbegin();
    }
    /** @brief Get an iterator to the first element of the vector */
    constexpr auto crbegin() const noexcept -> const_iterator
    {
        return val_.crbegin();
    }

    /**
     * @brief Get an iterator to one past the last element in the reverse vector
     */
    constexpr auto rend() noexcept -> iterator { return val_.rend(); }
    /**
     * @brief Get an iterator to one past the last element in the reverse vector
     */
    constexpr auto rend() const noexcept -> const_iterator
    {
        return val_.rend();
    }
    /**
     * @brief Get an iterator to one past the last element in the reverse vector
     */
    constexpr auto crend() const noexcept -> const_iterator
    {
        return val_.crend();
    }

    /** @brief Return whether the vector is empty (uninitialized) */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return val_.empty();
    }
    /** @brief Return the number of elements in the vector */
    constexpr auto size() const noexcept -> size_type { return val_.size(); }

    /** @brief Fill the vector with a value */
    constexpr auto fill(const T& value) -> void { val_.fill(value); }
    /** @brief Swap this vector with another vector */
    constexpr auto swap(Vec& other) noexcept -> void { val_.swap(other.val_); }

    /** @brief Equality comparison operator */
    auto operator==(const Vec& rhs) const -> bool { return val_ == rhs.val_; }
    /** @brief Inequality comparison operator */
    auto operator!=(const Vec& rhs) const -> bool { return val_ != rhs.val_; }

    /** @brief Assignment operator */
    template <class Vector>
    auto operator=(const Vector& b) -> Vec&
    {
        std::size_t idx{0};
        for (auto& v : val_) {
            v = b[idx++];
        }
        return *this;
    }

    /** @brief Assignment operator for std::initializer_list */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto operator=(const std::initializer_list<T2>& b) -> Vec&
    {
        auto it = b.begin();
        for (auto& v : val_) {
            v = *it;
            it++;
        }
        return *this;
    }

    /** @brief Addition assignment operator */
    template <class Vector>
    auto operator+=(const Vector& b) -> Vec&
    {
        std::size_t idx{0};
        for (auto& v : val_) {
            v += b[idx++];
        }
        return *this;
    }

    /** @brief Addition assignment operator for std::initializer_list */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto operator+=(const std::initializer_list<T2>& b) -> Vec&
    {
        auto it = b.begin();
        for (auto& v : val_) {
            v += *it;
            it++;
        }
        return *this;
    }

    /** @brief Addition operator */
    template <class Vector>
    friend auto operator+(Vec lhs, const Vector& rhs) -> Vec
    {
        lhs += rhs;
        return lhs;
    }

    /** @brief Subtraction assignment operator */
    template <class Vector>
    auto operator-=(const Vector& b) -> Vec&
    {
        std::size_t idx{0};
        for (auto& v : val_) {
            v -= b[idx++];
        }
        return *this;
    }

    /** @brief Subtraction assignment operator for std::initializer_list */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto operator-=(const std::initializer_list<T2>& b) -> Vec&
    {
        auto it = b.begin();
        for (auto& v : val_) {
            v -= *it;
            it++;
        }
        return *this;
    }

    /** @brief Subtraction operator */
    template <class Vector>
    friend auto operator-(Vec lhs, const Vector& rhs) -> Vec
    {
        lhs -= rhs;
        return lhs;
    }

    /** @brief Multiplication assignment operator */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto operator*=(const T2& b) -> Vec&
    {
        for (auto& v : val_) {
            v *= b;
        }
        return *this;
    }

    /** @brief Multiplication operator */
    template <class Vector>
    friend auto operator*(Vec lhs, const Vector& rhs) -> Vec
    {
        lhs *= rhs;
        return lhs;
    }

    /** @brief Division assignment operator */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto operator/=(const T2& b) -> Vec&
    {
        for (auto& v : val_) {
            v /= b;
        }
        return *this;
    }

    /** @brief Division operator */
    template <class Vector>
    friend auto operator/(Vec lhs, const Vector& rhs) -> Vec
    {
        lhs /= rhs;
        return lhs;
    }

    /** @brief Compute the vector dot product (i.e. inner product) */
    template <class Vector>
    auto dot(const Vector& v) -> T
    {
        return educelab::dot(val_, v);
    }

    /**
     * @brief Compute the vector dot product (i.e. inner product) with an
     * initializer list
     */
    template <
        typename T2,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto dot(const std::initializer_list<T2>& b) -> T
    {
        return educelab::dot(val_, b);
    }

    /** @brief Compute the vector cross product */
    template <class Vector, std::size_t D = Dims>
    auto cross(const Vector& v) -> std::enable_if_t<D == 3, Vec>
    {
        return educelab::cross(*this, v);
    }

    /** @brief Compute the vector cross product with an initializer list */
    template <
        typename T2,
        std::size_t D = Dims,
        std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
    auto cross(const std::initializer_list<T2>& b)
        -> std::enable_if_t<D == 3, Vec>
    {
        return educelab::cross(*this, b);
    }

    /** @brief Compute the vector magnitude */
    auto magnitude() const -> T { return educelab::norm(*this, Norm::L2); }

    /** @brief Return the unit vector of this vector */
    auto unit() const -> Vec { return educelab::normalize(*this); }

private:
    /** Values */
    Container val_{};
};

/** @brief 3D, 32-bit float vector */
using Vec3f = Vec<float, 3>;
/** @brief 3D, 64-bit float vector */
using Vec3d = Vec<double, 3>;

}  // namespace educelab

/** Debug: Print a vector to a std::ostream */
template <typename T, std::size_t Dims>
auto operator<<(std::ostream& os, const educelab::Vec<T, Dims>& vec)
    -> std::ostream&
{
    os << "[";
    std::size_t i{0};
    for (const auto& v : vec) {
        if (i++ > 0) {
            os << ", ";
        }
        os << v;
    }
    os << "]";
    return os;
}

namespace std
{
/**
 * @brief Hash function for educelab::Vec<T, 3>
 *
 * This hash function is designed for integral T types, so don't expect much if
 * using with floating-point types. Uses the hash function from:
 * https://dmauro.com/post/77011214305/a-hashing-function-for-x-y-z-coordinates
 */
template <typename T>
struct hash<educelab::Vec<T, 3>> {
    /** Hash Vec */
    auto operator()(educelab::Vec<T, 3> const& v) const noexcept -> std::size_t
    {
        auto max = std::max({v[0], v[1], v[2]});
        auto hash = (max * max * max) + (2 * max * v[2]) + v[2];
        if (max == v[2]) {
            auto val = std::max({v[0], v[1]});
            hash += val * val;
        }
        if (v[1] >= v[0]) {
            hash += v[0] + v[1];
        } else {
            hash += v[1];
        }
        return hash;
    }
};
}  // namespace std