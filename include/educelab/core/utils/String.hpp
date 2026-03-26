#pragma once

/** @file */

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace educelab
{

/** @brief Convert string characters to upper case (in place) */
static void to_upper(std::string& s)
{
    const auto& f = std::use_facet<std::ctype<char>>(std::locale());
    f.toupper(s.data(), s.data() + s.size());
}

/** @brief Convert string characters to upper case (r-value) */
static auto to_upper(std::string&& s) -> std::string
{
    to_upper(s);
    return std::move(s);
}

/** @brief Convert string characters to upper case (copy) */
static auto to_upper_copy(std::string s) -> std::string
{
    to_upper(s);
    return s;
}

/** @brief Convert string characters to lower case (in place) */
static void to_lower(std::string& s)
{
    const auto& f = std::use_facet<std::ctype<char>>(std::locale());
    f.tolower(s.data(), s.data() + s.size());
}

/** @brief Convert string characters to lower case (r-value) */
static auto to_lower(std::string&& s) -> std::string
{
    to_lower(s);
    return std::move(s);
}

/** @brief Convert string characters to lower case (copy) */
static auto to_lower_copy(std::string s) -> std::string
{
    to_lower(s);
    return s;
}

/** @brief Left trim */
static auto trim_left(std::string_view s) -> std::string_view
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
static void trim_left_in_place(std::string& s)
{
    const auto& loc = std::locale();
    s.erase(
        s.begin(),
        std::find_if_not(s.begin(), s.end(), [&loc](auto ch) -> bool {
            return std::isspace(ch, loc);
        }));
}

/** @brief Left trim (copy) */
static auto trim_left_copy(const std::string_view s) -> std::string
{
    return std::string{trim_left(s)};
}

/** @brief Right trim */
static auto trim_right(std::string_view s) -> std::string_view
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
static void trim_right_in_place(std::string& s)
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
static auto trim_right_copy(const std::string_view s) -> std::string
{
    return std::string{trim_right(s)};
}

/** @brief Trim from both ends */
static auto trim(std::string_view s) -> std::string_view
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
static void trim_in_place(std::string& s)
{
    trim_left_in_place(s);
    trim_right_in_place(s);
}

/** @brief Right trim (copy) */
static auto trim_copy(const std::string_view s) -> std::string
{
    return std::string{trim(s)};
}

/**
 * @brief Split a string wherever a predicate returns true, skipping
 * consecutive delimiter characters (no empty tokens produced)
 *
 * Clears @p tokens and fills with extracted tokens. Reuses the capacity of
 * @p tokens to avoid repeated heap allocations when the same vector is used
 * across many parsing iterations (e.g. in a per-line file-reading loop).
 * Single-pass O(n) core of the split family. Use when the delimiter is
 * expressible as a per-character predicate — character classes, single
 * characters, etc. For multi-character delimiters (e.g. @c "->") use the
 * string-delimiter overload.
 *
 * @code
 * // Split on slash — returns {"1", "2", "3"}
 * std::vector<std::string_view> tokens;
 * split("1/2/3", tokens, [](char c) { return c == '/'; });
 *
 * // Split on comma or semicolon
 * split("a,b;c", tokens, [](char c) { return c == ',' || c == ';'; });
 * @endcode
 */
template <
    typename Pred,
    std::enable_if_t<std::is_invocable_r_v<bool, Pred, char>, bool> = true>
static auto split(
    std::string_view s, std::vector<std::string_view>& tokens, Pred&& pred)
{
    tokens.clear();
    const auto* p = s.data();
    const auto* const end = p + s.size();
    while (p != end) {
        while (p != end && pred(*p)) {
            ++p;
        }
        if (p == end) {
            break;
        }
        const auto* const start = p;
        while (p != end && !pred(*p)) {
            ++p;
        }
        tokens.emplace_back(start, static_cast<std::size_t>(p - start));
    }
}

/**
 * @brief Split a string on any whitespace into a caller-provided vector
 *
 * @copydetails split
 */
static void split(std::string_view sv, std::vector<std::string_view>& tokens)
{
    split(sv, tokens, [](char c) {
        thread_local std::locale loc;
        return std::isspace(c, loc);
    });
}

/**
 * @brief Split a string by one or more string delimiters into a caller-provided
 * vector
 *
 * Clears @p tokens and fills with extracted tokens. Reuses the capacity of
 * @p tokens to avoid repeated heap allocations when the same vector is used
 * across many parsing iterations (e.g. in a per-line file-reading loop). When
 * all delimiters are single characters, dispatches to the O(n) predicate
 * overload automatically. Multi-character delimiters use the sort-based path.
 *
 * When provided conflicting delimiters, the largest delimiter takes
 * precedence:
 *
 * @code
 * std::vector<std::string_view> tokens;
 * split("a->b->c", tokens, "-", "->");  // returns {"a", "b", "c"}
 * @endcode
 */
template <typename... Ds>
static auto split(
    std::string_view s, std::vector<std::string_view>& tokens, const Ds&... ds)
{
    // Build delimiters list
    std::vector<std::string_view> delimiters{ds...};

    // Fast path: all delimiters are single characters — build a char set and
    // dispatch to the O(n) predicate overload
    if (std::all_of(delimiters.begin(), delimiters.end(), [](const auto& d) {
            return d.size() == 1;
        })) {
        std::vector<char> chars;
        chars.reserve(delimiters.size());
        for (const auto& d : delimiters) {
            chars.push_back(d[0]);
        }
        split(s, tokens, [&chars](char c) {
            return std::find(chars.begin(), chars.end(), c) != chars.end();
        });
        return;
    }

    // Get a list of all delimiter start pos and sizes
    std::vector<
        std::pair<std::string_view::size_type, std::string_view::size_type>>
        delimPos;
    for (const auto& delim : delimiters) {
        auto b = s.find(delim, 0);
        while (b != std::string_view::npos) {
            delimPos.emplace_back(b, delim.size());
            b = s.find(delim, b + delim.size());
        }
    }

    // Sort the delimiter start positions by first and largest
    std::sort(
        delimPos.begin(), delimPos.end(),
        [](const auto& l, const auto& r) { return l.second > r.second; });
    std::sort(
        delimPos.begin(), delimPos.end(),
        [](const auto& l, const auto& r) { return l.first < r.first; });

    // Split string
    tokens.clear();
    std::string_view::size_type begin{0};
    for (const auto& [end, size] : delimPos) {
        // ignore nested delimiters
        if (end < begin) {
            continue;
        }
        // get from begin to delim start
        if (auto t = s.substr(begin, end - begin); not t.empty()) {
            tokens.emplace_back(t);
        }
        begin = end + size;
    }
    if (auto t = s.substr(begin); not t.empty()) {
        tokens.emplace_back(t);
    }
}

/**
 * @brief Split a string wherever a predicate returns true, skipping
 * consecutive delimiter characters (no empty tokens produced)
 *
 * Single-pass O(n) core of the split family. Use when the delimiter is
 * expressible as a per-character predicate — character classes, single
 * characters, etc. For multi-character delimiters (e.g. @c "->") use the
 * string-delimiter overload.
 *
 * @tparam Pred Callable with signature @c bool(char)
 */
template <
    typename Pred,
    std::enable_if_t<std::is_invocable_r_v<bool, Pred, char>, bool> = true>
static auto split(std::string_view s, Pred&& pred)
    -> std::vector<std::string_view>
{
    std::vector<std::string_view> tokens;
    split(s, tokens, std::forward<Pred>(pred));
    return tokens;
}

/**
 * @brief Split a string on any whitespace, skipping consecutive whitespace
 *
 * Equivalent to Python's @c str.split() with no argument: any run of
 * whitespace characters (space, tab, carriage return, etc.) is treated as a
 * single delimiter, and leading/trailing whitespace produces no empty tokens.
 *
 * This is a single O(n) pass with no intermediate allocations for delimiter
 * positions. Prefer it for whitespace-delimited text such as OBJ/PLY lines.
 *
 * @code
 * // All return {"v", "1.0", "2.0", "3.0"}
 * split("v 1.0 2.0 3.0");
 * split("  v  1.0\t2.0  3.0  ");
 * @endcode
 */
static auto split(std::string_view s) -> std::vector<std::string_view>
{
    std::vector<std::string_view> tokens;
    split(s, tokens, [](char c) {
        thread_local std::locale loc;
        return std::isspace(c, loc);
    });
    return tokens;
}

/**
 * @brief Split a string by one or more string delimiters
 *
 * When provided conflicting delimiters, the largest delimiter takes
 * precedence:
 *
 * @code
 * split("a->b->c", "-", "->");  // returns {"a", "b", "c"}
 * @endcode
 *
 * When all delimiters are single characters, dispatches to the O(n) predicate
 * overload automatically. Multi-character delimiters use the sort-based path.
 */
template <typename... Ds>
static auto split(std::string_view s, const Ds&... ds)
    -> std::vector<std::string_view>
{
    std::vector<std::string_view> tokens;
    split(s, tokens, ds...);
    return tokens;
}

/** @brief Partition a string by a separator substring */
static auto partition(std::string_view s, const std::string_view sep)
    -> std::tuple<std::string_view, std::string_view, std::string_view>
{
    // Find the starting position
    const auto startPos = s.find(sep);

    // Didn't find the delimiter
    if (startPos == std::string::npos) {
        return {s, "", ""};
    }

    // Split into parts
    auto pre = s.substr(0, startPos);
    auto mid = s.substr(startPos, sep.size());
    auto post = s.substr(startPos + sep.size());

    // Return the parts
    return {pre, mid, post};
}

/** @brief Convert an Integer to a padded string */
template <
    typename Integer,
    std::enable_if_t<std::is_integral_v<Integer>, bool> = true>
auto to_padded_string(Integer val, const int padding, const char fill = '0')
    -> std::string
{
    std::stringstream stream;
    stream << std::setw(padding) << std::setfill(fill) << val;
    return stream.str();
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
auto to_numeric(const std::string_view str, Args... args) -> T
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

/**
 * @brief Convert a numeric value to a @c std::string_view into a
 * caller-provided buffer, with no heap allocation
 *
 * Wraps @c std::to_chars to eliminate boilerplate. The caller declares one
 * buffer and reuses it across all conversions in a serialization pass:
 *
 * @code
 * std::array<char, 128> buf;
 * for (const auto& v : mesh.vertices()) {
 *     file << to_string_view(buf, v[0]) << ' '
 *          << to_string_view(buf, v[1]) << ' '
 *          << to_string_view(buf, v[2]) << '\n';
 * }
 * @endcode
 *
 * @warning The returned @c std::string_view is only valid until the next call
 * to @c to_string_view (or any other write) using the same buffer.
 *
 * @throws std::runtime_error if @p buf is too small for the converted value
 * @tparam N Buffer size; 128 is sufficient for any standard arithmetic type
 * @tparam T Arithmetic type to convert
 */
template <
    std::size_t N,
    typename T,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
auto to_string_view(std::array<char, N>& buf, T val) -> std::string_view
{
    const auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + N, val);
    if (ec != std::errc{}) {
        throw std::runtime_error("to_string_view: numeric conversion failed");
    }
    return std::string_view(buf.data(), static_cast<std::size_t>(ptr - buf.data()));
}
/**
 * @brief Convert a numeric value to a @c std::string without locale dependency
 *
 * Convenience wrapper over @ref to_string_view for one-off conversions where
 * buffer reuse is not needed. Allocates a @c std::string on each call — for
 * hot paths (e.g. per-vertex coordinate output in a write loop), declare a
 * buffer and use @ref to_string_view directly.
 *
 * @tparam T Arithmetic type to convert
 */
template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
auto to_string(T val) -> std::string
{
    std::array<char, 128> buf{};
    return std::string(to_string_view(buf, val));
}

// --- to_string_view fallbacks (when std::to_chars is unavailable per type) ---

#ifdef EDUCE_CORE_NEED_TO_CHARS_FLOAT
/**
 * @brief Specialisation of @ref to_string_view for @c float on platforms
 *        where @c std::to_chars does not support @c float.
 * Uses @c snprintf as a fallback.
 */
template <std::size_t N>
auto to_string_view(std::array<char, N>& buf, float val) -> std::string_view
{
    const int n = std::snprintf(buf.data(), N, "%.9g", static_cast<double>(val));
    if (n < 0 || static_cast<std::size_t>(n) >= N) {
        throw std::runtime_error("to_string_view: numeric conversion failed");
    }
    return std::string_view(buf.data(), static_cast<std::size_t>(n));
}
#endif

#ifdef EDUCE_CORE_NEED_TO_CHARS_DOUBLE
/**
 * @brief Specialisation of @ref to_string_view for @c double on platforms
 *        where @c std::to_chars does not support @c double.
 * Uses @c snprintf as a fallback.
 */
template <std::size_t N>
auto to_string_view(std::array<char, N>& buf, double val) -> std::string_view
{
    const int n = std::snprintf(buf.data(), N, "%.17g", val);
    if (n < 0 || static_cast<std::size_t>(n) >= N) {
        throw std::runtime_error("to_string_view: numeric conversion failed");
    }
    return std::string_view(buf.data(), static_cast<std::size_t>(n));
}
#endif

#ifdef EDUCE_CORE_NEED_TO_CHARS_LONG_DOUBLE
/**
 * @brief Specialisation of @ref to_string_view for @c long double on platforms
 *        where @c std::to_chars does not support @c long double.
 *
 * Casts to @c double before conversion. On Apple platforms @c long double
 * has the same 64-bit representation as @c double, so no precision is lost.
 */
template <std::size_t N>
auto to_string_view(std::array<char, N>& buf, long double val) -> std::string_view
{
    return to_string_view(buf, static_cast<double>(val));
}
#endif

// --- to_numeric fallbacks (when std::from_chars is unavailable per type) ----

#ifdef EDUCE_CORE_NEED_FROM_CHARS_FLOAT
/**
 * @copybrief to_numeric
 *
 * Template specialization active when @c EDUCE_CORE_NEED_FROM_CHARS_FLOAT is
 * defined (i.e. @c std::from_chars is unavailable for @c float on this
 * platform). Converts via @c std::stof instead.
 */
template <>
inline auto to_numeric<float>(const std::string_view str) -> float
{
    return std::stof(std::string(str));
}
#endif

#ifdef EDUCE_CORE_NEED_FROM_CHARS_DOUBLE
/**
 * @copybrief to_numeric
 *
 * Template specialization active when @c EDUCE_CORE_NEED_FROM_CHARS_DOUBLE is
 * defined. Converts via @c std::stod instead.
 */
template <>
inline auto to_numeric<double>(const std::string_view str) -> double
{
    return std::stod(std::string(str));
}
#endif

#ifdef EDUCE_CORE_NEED_FROM_CHARS_LONG_DOUBLE
/**
 * @copybrief to_numeric
 *
 * Template specialization active when @c EDUCE_CORE_NEED_FROM_CHARS_LONG_DOUBLE
 * is defined. Converts via @c std::stold instead.
 */
template <>
inline auto to_numeric<long double>(const std::string_view str) -> long double
{
    return std::stold(std::string(str));
}
#endif

}  // namespace educelab
