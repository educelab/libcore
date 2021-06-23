#include <gmock/gmock.h>
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

TEST(String, TrimLeftInPlace)
{
    std::string result{"    This is only a test.    "};
    trim_left(result);
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimLeftRValue)
{
    auto result = trim_left("    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimLeftCopy)
{
    std::string input{"    This is only a test.    "};
    auto result = trim_left_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.    ");
}

TEST(String, TrimRightInPlace)
{
    std::string result{"    This is only a test.    "};
    trim_right(result);
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, TrimRightRValue)
{
    auto result = trim_right("    This is only a test.    ");
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, TrimRightCopy)
{
    std::string input{"    This is only a test.    "};
    auto result = trim_right_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "    This is only a test.");
}

TEST(String, TrimInPlace)
{
    std::string result{"    This is only a test.    "};
    trim(result);
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, TrimRValue)
{
    auto result = trim("    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, TrimCopy)
{
    std::string input{"    This is only a test.    "};
    auto result = trim_copy(input);
    EXPECT_EQ(input, "    This is only a test.    ");
    EXPECT_EQ(result, "This is only a test.");
}

TEST(String, SplitDefault)
{
    std::string input{" This is only a test. "};
    std::vector<std::string> expected{"This", "is", "only", "a", "test."};
    auto result = split(input);
    using ::testing::ContainerEq;
    EXPECT_THAT(result, ContainerEq(expected));
}