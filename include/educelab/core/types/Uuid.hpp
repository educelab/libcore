#pragma once

/** @file */

#include <array>
#include <string>

namespace educelab
{

/**
 * @brief Universally unique identifier class
 *
 * Attempts to implement the UUID defined by RFC 4122.
 */
class Uuid
{
public:
    /** Byte type */
    using Byte = uint8_t;

    /** Default constructor. Constructed UUID is nil valued. */
    Uuid() = default;

    /** Functor: Returns true if not nil */
    explicit operator bool() const;
    /** Equality: Returns true if UUID bytes are the same */
    auto operator==(const Uuid& rhs) const -> bool;
    /** Inequality: Returns true if UUID bytes are not the same */
    auto operator!=(const Uuid& rhs) const -> bool;

    /** @brief Reset the UUID to a nil value */
    void reset();
    /** @brief Returns true is all bytes are zero */
    auto is_nil() const -> bool;

    /**
     * @brief Get a string representation of the UUID
     *
     * Gets a string representation of the UUID as 16 hexadecimal digits:
     * aabbccdd-eeff-0011-2233-445566778899
     */
    auto string() const -> std::string;

    /**
     * @brief Construct a UUID from a string
     *
     *  @throws std::invalid_argument if str is not of the form:
     *  aabbccdd-eeff-0011-2233-445566778899
     */
    static auto FromString(const std::string& str) -> Uuid;

    /**
     * @brief Generate a UUIDv4 using pseudo-random numbers
     *
     * See RFC 4122 section 4.4 for more details.
     */
    static auto Uuid4() -> Uuid;

private:
    /** Byte storage */
    std::array<Byte, 16> buffer_{};
};

}  // namespace educelab

namespace std
{
/** @brief Hash function for educelab::Uuid */
template <>
struct hash<educelab::Uuid> {
    /** Hash Uuid */
    auto operator()(educelab::Uuid const& u) const noexcept -> std::size_t
    {
        return std::hash<std::string>{}(u.string());
    }
};
}  // namespace std