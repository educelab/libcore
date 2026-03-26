#include <gtest/gtest.h>

#include <string>
#include <tuple>

#include "educelab/core/utils/String.hpp"

using namespace educelab;

TEST(String, ToUpperInPlace)
{
    std::string result{"This is only a test."};
    to_upper(result);
    EXPECT_EQ(result, "THIS IS ONLY A TEST.");
}

TEST(String, ToUpperRValue)
{
    const auto result = to_upper("This is only a test.");
    EXPECT_EQ(result, "THIS IS ONLY A TEST.");
}

TEST(String, ToUpperCopy)
{
    const std::string input{"This is only a test."};
    const auto result = to_upper_copy(input);
    EXPECT_EQ(input, "This is only a test.");
    EXPECT_EQ(result, "THIS IS ONLY A TEST.");
}

TEST(String, ToLowerInPlace)
{
    std::string result{"This is only a test."};
    to_lower(result);
    EXPECT_EQ(result, "this is only a test.");
}

TEST(String, ToLowerRValue)
{
    const auto result = to_lower("This is only a test.");
    EXPECT_EQ(result, "this is only a test.");
}

TEST(String, ToLowerCopy)
{
    const std::string input{"This is only a test."};
    const auto result = to_lower_copy(input);
    EXPECT_EQ(input, "This is only a test.");
    EXPECT_EQ(result, "this is only a test.");
}

TEST(String, TrimLeft)
{
    const std::string input{"    This is only a test.    "};
    auto result = trim_left(input);
    EXPECT_NE(result, input);
    EXPECT_EQ(result, "This is only a test.    ");

    result = trim_left("    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimLeftInPlace)
{
    std::string result{"    This is only a test.    "};
    trim_left_in_place(result);
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimLeftCopy)
{
    const std::string input{"    This is only a test.    "};
    const auto result = trim_left_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimRight)
{
    const std::string input{"    This is only a test.    "};
    auto result = trim_right(input);
    EXPECT_NE(result, input);
    EXPECT_EQ(result, "    This is only a test.");

    result = trim_right("    This is only a test.    ");
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, TrimRightInPlace)
{
    std::string result{"    This is only a test.    "};
    trim_right_in_place(result);
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, TrimRightCopy)
{
    const std::string input{"    This is only a test.    "};
    const auto result = trim_right_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, Trim)
{
    const std::string input{"    This is only a test.    "};
    auto result = trim(input);
    EXPECT_NE(result, input);
    EXPECT_EQ(result, "This is only a test.");

    result = trim("    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, TrimInPlace)
{
    std::string result{"    This is only a test.    "};
    trim_in_place(result);
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, TrimCopy)
{
    const std::string input{"    This is only a test.    "};
    const auto result = trim_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, SplitInto)
{
    // Setup expected
    std::vector<std::string_view> expected{"a", "b", "c"};

    // Result vector
    std::vector<std::string_view> result;

    // Space separated (predicate)
    split("a b c", result, [](char c) {
        thread_local std::locale loc;
        return std::isspace(c, loc);
    });
    EXPECT_EQ(result, expected);
    result.clear();

    // Space separated (implicit)
    split("a b c", result);
    EXPECT_EQ(result, expected);
    result.clear();

    // Multiple spaces (implicit)
    split("  a  b  c  ", result);
    EXPECT_EQ(result, expected);
    result.clear();

    // Space separated (explicit)
    split("a b c", result, " ");
    EXPECT_EQ(result, expected);
    result.clear();

    // Comma separated
    split("a,b,c", result, ",");
    EXPECT_EQ(result, expected);
    result.clear();

    // Multi-delimited
    split("a+b-c", result, "+", "-");
    EXPECT_EQ(result, expected);
    result.clear();

    // Multi-character delimiter
    split("a b->c", result, " ", "->");
    EXPECT_EQ(result, expected);
    result.clear();

    // Multi-character, ignore nested
    split("a-b->c", result, "-", "->");
    EXPECT_EQ(result, expected);
    result.clear();

    // Overlapping will only consume first delim
    expected = {"a", "b", ">c"};
    split("a--b-->c", result, "--", "->");
    EXPECT_EQ(result, expected);
    result.clear();

    // Sentence
    expected = {"This", "is", "only", "a", "test."};
    split("This is only a test.", result);
    EXPECT_EQ(result, expected);
    result.clear();
}

TEST(String, Split)
{
    // Setup expected
    std::vector<std::string_view> expected{"a", "b", "c"};

    // Space separated (predicate)
    auto result = split("a b c", [](char c) {
        thread_local std::locale loc;
        return std::isspace(c, loc);
    });
    EXPECT_EQ(result, expected);

    // Space separated (implicit)
    EXPECT_EQ(split("a b c"), expected);

    // Multiple spaces (implicit)
    EXPECT_EQ(split("  a  b  c  "), expected);

    // Space separated (explicit)
    EXPECT_EQ(split("a b c", " "), expected);

    // Comma separated
    EXPECT_EQ(split("a,b,c", ","), expected);

    // Multi-delimited
    EXPECT_EQ(split("a+b-c", "+", "-"), expected);

    // Multi-character delimiter
    EXPECT_EQ(split("a b->c", " ", "->"), expected);

    // Multi-character, ignore nested
    EXPECT_EQ(split("a-b->c", "-", "->"), expected);

    // Overlapping will only consume first delim
    expected = {"a", "b", ">c"};
    EXPECT_EQ(split("a--b-->c", "--", "->"), expected);

    // Sentence
    expected = {"This", "is", "only", "a", "test."};
    EXPECT_EQ(split("This is only a test."), expected);
}

TEST(String, SplitEdgeCases)
{
    std::vector<std::string_view> result;

    // Empty string
    split("", result);
    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(split("").empty());

    // No delimiters found
    split("abc", result, ",");
    EXPECT_EQ(result, std::vector<std::string_view>{"abc"});
    EXPECT_EQ(split("abc", ","), std::vector<std::string_view>{"abc"});

    // Leading/trailing/consecutive delimiters (whitespace/predicate)
    // Should skip all empty tokens
    std::vector<std::string_view> expected{"a", "b"};
    split("  a  b  ", result);
    EXPECT_EQ(result, expected);
    EXPECT_EQ(split("  a  b  "), expected);

    // Leading/trailing/consecutive delimiters (string)
    split("--a--b--", result, "-");
    EXPECT_EQ(result, expected);
    EXPECT_EQ(split("--a--b--", "-"), expected);

    // Predicate always true
    split("abc", result, [](char) { return true; });
    EXPECT_TRUE(result.empty());

    // Predicate always false
    split("abc", result, [](char) { return false; });
    EXPECT_EQ(result, std::vector<std::string_view>{"abc"});

    // No delimiters provided to variadic split
    // (Note: this should compile and return the whole string as one token)
    EXPECT_EQ(split("abc"), std::vector<std::string_view>{"abc"});
}

TEST(String, Partition)
{
    // key=value
    const auto [p0, m0, s0] = partition("key=value", "=");
    EXPECT_EQ(p0, "key");
    EXPECT_EQ(m0, "=");
    EXPECT_EQ(s0, "value");

    // {} replacement
    const auto [p1, m1, s1] = partition("prefix{}suffix", "{}");
    EXPECT_EQ(p1, "prefix");
    EXPECT_EQ(m1, "{}");
    EXPECT_EQ(s1, "suffix");

    // to existing strings
    std::string p2, m2, s2;
    std::tie(p2, m2, s2) = partition("abc", "b");
    EXPECT_EQ(p2, "a");
    EXPECT_EQ(m2, "b");
    EXPECT_EQ(s2, "c");
}

TEST(String, ToPaddedString)
{
    // zero-pad (default)
    EXPECT_EQ(to_padded_string(5, 3), "005");
    EXPECT_EQ(to_padded_string(5, 10), "0000000005");

    // zero-pad (custom)
    EXPECT_EQ(to_padded_string(5, 3, '5'), "555");
    EXPECT_EQ(to_padded_string(5, 3, 'x'), "xx5");
}

TEST(String, ToNumeric)
{
    std::string test{"100.3456 unparsed"};
    EXPECT_EQ(to_numeric<int8_t>(test), std::int8_t{100});
    EXPECT_EQ(to_numeric<uint8_t>(test), std::uint8_t{100});
    EXPECT_EQ(to_numeric<int16_t>(test), std::int16_t{100});
    EXPECT_EQ(to_numeric<uint16_t>(test), std::uint16_t{100});
    EXPECT_EQ(to_numeric<int32_t>(test), std::int32_t{100});
    EXPECT_EQ(to_numeric<uint32_t>(test), std::uint32_t{100});
    EXPECT_EQ(to_numeric<int64_t>(test), std::int64_t{100});
    EXPECT_EQ(to_numeric<uint64_t>(test), std::uint64_t{100});

    EXPECT_EQ(to_numeric<float>(test), 100.3456F);
    EXPECT_EQ(to_numeric<double>(test), 100.3456);
    EXPECT_EQ(to_numeric<long double>(test), 100.3456L);

    EXPECT_THROW(to_numeric<int>("bad"), std::invalid_argument);
    EXPECT_THROW(to_numeric<uint8_t>("256"), std::out_of_range);
}
