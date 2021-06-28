#pragma once

/** @file */

#include <algorithm>
#include <charconv>
#include <exception>
#include <locale>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace educelab
{

/** @brief Convert string characters to upper case (in place) */
static inline void to_upper(std::string& s)
{
    const auto& f = std::use_facet<std::ctype<char>>(std::locale());
    f.toupper(s.data(), s.data() + s.size());
}

/** @brief Convert string characters to upper case (r-value) */
static inline auto to_upper(std::string&& s) -> std::string
{
    to_upper(s);
    return std::move(s);
}

/** @brief Convert string characters to upper case (copy) */
static inline auto to_upper_copy(std::string s) -> std::string
{
    to_upper(s);
    return s;
}

/** @brief Convert string characters to lower case (in place) */
static inline void to_lower(std::string& s)
{
    const auto& f = std::use_facet<std::ctype<char>>(std::locale());
    f.tolower(s.data(), s.data() + s.size());
}

/** @brief Convert string characters to lower case (r-value) */
static inline auto to_lower(std::string&& s) -> std::string
{
    to_lower(s);
    return std::move(s);
}

/** @brief Convert string characters to lower case (copy) */
static inline auto to_lower_copy(std::string s) -> std::string
{
    to_lower(s);
    return s;
}

/** @brief Left trim */
static inline auto trim_left(std::string_view s) -> std::string_view
{
    const auto& loc = std::locale();
    const auto* start = std::find_if_not(
        std::begin(s), std::end(s),
        [&loc](auto ch) -> bool { return std::isspace(ch, loc); });
    s.remove_prefix(std::distance(std::begin(s), start));
    return s;
}

/**
 * @brief Left trim (in place)
 *
 * https://stackoverflow.com/a/217605
 */
static inline void trim_left_in_place(std::string& s)
{
    const auto& loc = std::locale();
    s.erase(
        s.begin(),
        std::find_if_not(s.begin(), s.end(), [&loc](auto ch) -> bool {
            return std::isspace(ch, loc);
        }));
}

/** @brief Left trim (copy) */
static inline auto trim_left_copy(std::string_view s) -> std::string
{
    return std::string{trim_left(s)};
}

/** @brief Right trim */
static inline auto trim_right(std::string_view s) -> std::string_view
{
    const auto& loc = std::locale();
    const auto* start =
        std::find_if_not(s.rbegin(), s.rend(), [&loc](auto ch) -> bool {
            return std::isspace(ch, loc);
        }).base();
    s.remove_suffix(std::distance(start, std::end(s)));
    return s;
}

/**
 * @brief Right trim (in place)
 *
 * https://stackoverflow.com/a/217605
 */
static inline void trim_right_in_place(std::string& s)
{
    const auto& loc = std::locale();
    s.erase(
        std::find_if_not(
            s.rbegin(), s.rend(),
            [&loc](auto ch) -> bool { return std::isspace(ch, loc); })
            .base(),
        s.end());
}

/** @brief Right trim (copy) */
static inline auto trim_right_copy(std::string_view s) -> std::string
{
    return std::string{trim_right(s)};
}

/** @brief Trim from both ends */
static inline auto trim(std::string_view s) -> std::string_view
{
    s = trim_left(s);
    s = trim_right(s);
    return s;
}

/**
 * @brief Trim from both ends (in place)
 *
 * https://stackoverflow.com/a/217605
 */
static inline void trim_in_place(std::string& s)
{
    trim_left_in_place(s);
    trim_right_in_place(s);
}

/** @brief Right trim (copy) */
static inline auto trim_copy(std::string_view s) -> std::string
{
    return std::string{trim(s)};
}

/** @brief Split a string by a delimiter */
template <typename... Ds>
static inline auto split(std::string_view s, const Ds&... ds)
    -> std::vector<std::string_view>
{
    // Build delimiters list
    std::vector<char> delimiters;
    if (sizeof...(ds) > 0) {
        delimiters = {ds...};
    } else {
        delimiters.emplace_back(' ');
    }

    // Get a list of all delimiter start positions
    std::vector<std::string_view::size_type> delimPos;
    for (const auto& delim : delimiters) {
        auto b = s.find(delim, 0);
        while (b != std::string_view::npos) {
            delimPos.emplace_back(b);
            b = s.find(delim, b + 1);
        }
    }

    // Sort the delimiter start positions
    std::sort(delimPos.begin(), delimPos.end());

    // Split string
    std::vector<std::string_view> tokens;
    std::string_view::size_type begin{0};
    for (auto end : delimPos) {
        auto t = s.substr(begin, end - begin);
        if (not t.empty()) {
            tokens.emplace_back(t);
        }
        begin = end + 1;
    }
    auto t = s.substr(begin);
    if (not t.empty()) {
        tokens.emplace_back(t);
    }

    return tokens;
}

/**
 * @brief Convert a string to a numeric type.
 *
 * A drop-in replacement for the `std:sto` family of functions which uses
 * `std::from_chars` for conversion. Like `std::sto`, throws exceptions when
 * conversion fails or if the converted value is out of range of the result
 * type.
 *
 * @throws std::invalid_argument If string cannot be converted to the result
 * type.
 * @throws std::result_out_of_range If converted value is out of range for the
 * result type.
 * @tparam T Requested numeric type
 * @tparam Args Parameter pack type
 * @param str Value to convert
 * @param args Extra parameters passed directly to `std::to_chars`
 * @return Converted value
 */
template <typename T, typename... Args>
auto to_numeric(std::string_view str, Args... args) -> T
{
    T val;
    const auto* first = std::data(str);
    const auto* last = std::data(str) + std::size(str);
    auto [ptr, ec] = std::from_chars(first, last, val, args...);
    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("Conversion could not be performed");
    }
    if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("Value out of range for the result type");
    }
    return val;
}

#ifdef EDUCELAB_NEED_TO_NUMERIC_FP
/**
 * @copybrief to_numeric
 *
 * Template specialization as fallback when the compiler does not support
 * `std::from_chars` for floating point types. Converts the input to a
 * `std::string` and passes to the appropriate `std::sto` function.
 */
template <>
auto to_numeric<float>(std::string_view str) -> float
{
    return std::stof(std::string(str));
}

/** @copydoc to_numeric<float> */
template <>
auto to_numeric<double>(std::string_view str) -> double
{
    return std::stod(std::string(str));
}

/** @copydoc to_numeric<float> */
template <>
auto to_numeric<long double>(std::string_view str) -> long double
{
    return std::stold(std::string(str));
}
#endif

}  // namespace educelab
