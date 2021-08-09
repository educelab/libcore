#include "educelab/core/types/Uuid.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

using namespace educelab;

namespace detail
{
/**
 * @brief Create a random number engine
 *
 * Constructs a random number engine seeded with a random sequence to improve
 * entropy.
 *
 * Solution from: https://codereview.stackexchange.com/a/109266
 */
template <
    class T = std::mt19937,
    std::size_t N = T::state_size * sizeof(typename T::result_type)>
auto SeededRandomEngine() -> typename std::enable_if<N != 0, T>::type
{
    std::random_device source;
    using ArrayT = std::random_device::result_type;
    std::array<ArrayT, (N - 1) / sizeof(source()) + 1> data{};
    std::generate(std::begin(data), std::end(data), std::ref(source));
    std::seed_seq seeds(std::begin(data), std::end(data));
    return T(seeds);
}
}  // namespace detail

Uuid::operator bool() const { return is_nil(); }

auto Uuid::operator==(const Uuid& rhs) const -> bool
{
    return buffer_ == rhs.buffer_;
}

auto Uuid::operator!=(const Uuid& rhs) const -> bool
{
    return buffer_ != rhs.buffer_;
}

void Uuid::reset() { buffer_.fill(0); }

auto Uuid::is_nil() const -> bool
{
    return std::all_of(
        buffer_.begin(), buffer_.end(), [](const auto& v) { return v == 0; });
}

auto Uuid::string() const -> std::string
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t idx = 0; idx < buffer_.size(); idx++) {
        if (idx == 4 or idx == 6 or idx == 8 or idx == 10) {
            ss << '\u002D';
        }
        ss << std::setw(2) << static_cast<unsigned>(buffer_[idx]);
    }
    return ss.str();
}

auto Uuid::FromString(const std::string& str) -> Uuid
{
    // TODO: 13th character (first digit of 7th byte) is version number
    static std::regex uuidRe{
        R"([a-f0-9]{8}\u002D[a-f0-9]{4}\u002D[a-f0-9]{4}\u002D[a-f0-9]{4}\u002D[a-f0-9]{12})"};
    if (not std::regex_match(str, uuidRe)) {
        throw std::invalid_argument("Provided string not a valid Uuid");
    }

    // Make new uuid
    Uuid uuid;

    // Iterate over string
    size_t strIdx{0};
    for (auto& byte : uuid.buffer_) {
        // Skip the dashes
        if (str[strIdx] == '\u002D') {
            strIdx++;
        }

        // Get the next two chars and convert
        auto byteStr = str.substr(strIdx, 2);
        byte = std::strtoul(byteStr.c_str(), nullptr, 16);
        strIdx += 2;
    }

    return uuid;
}

auto Uuid::Uuid4() -> Uuid
{
    // Make new uuid
    Uuid uuid;

    // Create the random engine and distribution
    static auto rand = detail::SeededRandomEngine();
    static std::uniform_int_distribution<Uuid::Byte> dist;

    // Generate random bytes
    for (auto& v : uuid.buffer_) {
        v = dist(rand);
    }

    // Set the v4 bit fields: https://www.cryptosys.net/pki/Uuid.c.html
    uuid.buffer_[6] = 0x40u | (uuid.buffer_[6] & 0xfu);
    uuid.buffer_[8] = 0x80u | (uuid.buffer_[8] & 0x3fu);

    return uuid;
}