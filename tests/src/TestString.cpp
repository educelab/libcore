#include <gtest/gtest.h>

#include <string>

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
    auto result = to_upper("This is only a test.");
    EXPECT_EQ(result, "THIS IS ONLY A TEST.");
}

TEST(String, ToUpperCopy)
{
    std::string input{"This is only a test."};
    auto result = to_upper_copy(input);
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
    auto result = to_lower("This is only a test.");
    EXPECT_EQ(result, "this is only a test.");
}

TEST(String, ToLowerCopy)
{
    std::string input{"This is only a test."};
    auto result = to_lower_copy(input);
    EXPECT_EQ(input, "This is only a test.");
    EXPECT_EQ(result, "this is only a test.");
}

TEST(String, TrimLeft)
{
    std::string input{"    This is only a test.    "};
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
    std::string input{"    This is only a test.    "};
    auto result = trim_left_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimRight)
{
    std::string input{"    This is only a test.    "};
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
    std::string input{"    This is only a test.    "};
    auto result = trim_right_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, Trim)
{
    std::string input{"    This is only a test.    "};
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
    std::string input{"    This is only a test.    "};
    auto result = trim_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, Split)
{
    // Setup expected
    std::vector<std::string_view> expected{"a", "b", "c"};

    // Space separated (implicit)
    EXPECT_EQ(split("a b c"), expected);

    // Multiple spaces (implicit)
    EXPECT_EQ(split("  a  b  c  "), expected);

    // Space separated (explicit)
    EXPECT_EQ(split("a b c", ' '), expected);

    // Comma separated
    EXPECT_EQ(split("a,b,c", ','), expected);

    // Multi-delimited
    EXPECT_EQ(split("a+b-c", '+', '-'), expected);

    // Sentence
    expected = {"This", "is", "only", "a", "test."};
    EXPECT_EQ(split("This is only a test."), expected);
}

TEST(String, ToNumeric)
{
    std::string test{"100.3456 unparsed"};
    EXPECT_EQ(to_numeric<int8_t>(test), int8_t(100));
    EXPECT_EQ(to_numeric<uint8_t>(test), uint8_t(100));
    EXPECT_EQ(to_numeric<int16_t>(test), int16_t(100));
    EXPECT_EQ(to_numeric<uint16_t>(test), uint16_t(100));
    EXPECT_EQ(to_numeric<int32_t>(test), int32_t(100));
    EXPECT_EQ(to_numeric<uint32_t>(test), uint32_t(100));
    EXPECT_EQ(to_numeric<int64_t>(test), int64_t(100));
    EXPECT_EQ(to_numeric<uint64_t>(test), uint64_t(100));

    EXPECT_EQ(to_numeric<float>(test), 100.3456F);
    EXPECT_EQ(to_numeric<double>(test), 100.3456);
    EXPECT_EQ(to_numeric<long double>(test), 100.3456L);

    EXPECT_THROW(to_numeric<int>("bad"), std::invalid_argument);
    EXPECT_THROW(to_numeric<uint8_t>("256"), std::out_of_range);
}
