#include <gtest/gtest.h>

#include "educelab/core/utils/Filesystem.hpp"

using namespace educelab;
namespace fs = std::filesystem;

TEST(Filesystem, IsFileTypeString)
{
    // Single matching extension
    EXPECT_TRUE(is_file_type("some/path.jpg", "jpg"));
    // Double matching extension
    EXPECT_TRUE(is_file_type("some/path.jpg", "tif", "jpg"));

    // Single non-matching extension
    EXPECT_FALSE(is_file_type("some/path.jpg", "tif"));
    // Double non-matching extension
    EXPECT_FALSE(is_file_type("some/path.jpg", "tif", "bmp"));

    // No extension
    EXPECT_FALSE(is_file_type("file", ""));
}

TEST(Filesystem, IsFileTypePath)
{
    // Path
    fs::path input{"some/path.jpg"};

    // Single matching extension
    EXPECT_TRUE(is_file_type(input, "jpg"));
    // Double matching extension
    EXPECT_TRUE(is_file_type(input, "tif", "jpg"));

    // Single non-matching extension
    EXPECT_FALSE(is_file_type(input, "tif"));
    // Double non-matching extension
    EXPECT_FALSE(is_file_type(input, "tif", "bmp"));

    // No extension
    input = "file";
    EXPECT_FALSE(is_file_type(input, ""));
}