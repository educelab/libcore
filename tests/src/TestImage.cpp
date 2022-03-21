#include <gtest/gtest.h>

#include "educelab/core/types/Image.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

TEST(Image, DefaultConstructor)
{
    Image img;
    EXPECT_EQ(img.height(), 0);
    EXPECT_EQ(img.width(), 0);
    EXPECT_EQ(img.channels(), 0);
    EXPECT_EQ(img.type(), Depth::None);
    EXPECT_FALSE(img);
}

TEST(Image, PropertiesConstructorU8)
{
    // Construct image
    Image img(5, 10, 1, Depth::U8);
    EXPECT_EQ(img.height(), 5);
    EXPECT_EQ(img.width(), 10);
    EXPECT_EQ(img.channels(), 1);
    EXPECT_EQ(img.type(), Depth::U8);
    EXPECT_TRUE(img);

    // Check zero initialization
    for (const auto [y, x] : range2D(img.height(), img.width())) {
        EXPECT_EQ(img.at<std::uint8_t>(y, x), std::uint8_t{0});
    }
}

TEST(Image, PropertiesConstructorU16)
{
    // Construct image
    Image img(5, 10, 1, Depth::U16);
    EXPECT_EQ(img.height(), 5);
    EXPECT_EQ(img.width(), 10);
    EXPECT_EQ(img.channels(), 1);
    EXPECT_EQ(img.type(), Depth::U16);
    EXPECT_TRUE(img);

    // Check zero initialization
    for (const auto [y, x] : range2D(img.height(), img.width())) {
        EXPECT_EQ(img.at<std::uint16_t>(y, x), std::uint16_t{0});
    }
}

TEST(Image, PropertiesConstructorF32)
{
    // Construct image
    Image img(5, 10, 1, Depth::F32);
    EXPECT_EQ(img.height(), 5);
    EXPECT_EQ(img.width(), 10);
    EXPECT_EQ(img.channels(), 1);
    EXPECT_EQ(img.type(), Depth::F32);
    EXPECT_TRUE(img);

    // Check zero initialization
    for (const auto [y, x] : range2D(img.height(), img.width())) {
        EXPECT_FLOAT_EQ(img.at<float>(y, x), 0.F);
    }
}

TEST(Image, ConvertFromU8)
{
    // Construct image
    Image img(10, 10, 1, Depth::U8);
    for (const auto i : range(10)) {
        img.at<std::uint8_t>(i, i) = 255;
        img.at<std::uint8_t>(i, 9 - i) = 127;
    }

    // Convert to 16-bpc
    auto img16 = Image::Convert(img, Depth::U16);
    for (const auto i : range(10)) {
        EXPECT_EQ(img16.at<std::uint16_t>(i, i), 65535);
        EXPECT_EQ(img16.at<std::uint16_t>(i, 9 - i), 32639);
    }

    // Convert to 32-bpc
    auto img32 = Image::Convert(img, Depth::F32);
    for (const auto i : range(10)) {
        EXPECT_FLOAT_EQ(img32.at<float>(i, i), 1.F);
        EXPECT_FLOAT_EQ(img32.at<float>(i, 9 - i), 127.F / 255.F);
    }
}

TEST(Image, ConvertFromU16)
{
    // Construct image
    Image img(10, 10, 1, Depth::U16);
    for (const auto i : range(10)) {
        img.at<std::uint16_t>(i, i) = 65535;
        img.at<std::uint16_t>(i, 9 - i) = 32767;
    }

    // Convert to 8-bpc
    auto img8 = Image::Convert(img, Depth::U8);
    for (const auto i : range(10)) {
        EXPECT_EQ(img8.at<std::uint8_t>(i, i), 255);
        EXPECT_EQ(img8.at<std::uint8_t>(i, 9 - i), 127);
    }

    // Convert to 32-bpc
    auto img32 = Image::Convert(img, Depth::F32);
    for (const auto i : range(10)) {
        EXPECT_FLOAT_EQ(img32.at<float>(i, i), 1.F);
        EXPECT_FLOAT_EQ(img32.at<float>(i, 9 - i), 32767.F / 65535.F);
    }
}

TEST(Image, ConvertFromF32)
{
    // Construct image
    Image img(10, 10, 1, Depth::F32);
    for (const auto i : range(10)) {
        img.at<float>(i, i) = 1.F;
        img.at<float>(i, 9 - i) = 0.5F;
    }

    // Convert to 8-bpc
    auto img8 = Image::Convert(img, Depth::U8);
    for (const auto i : range(10)) {
        EXPECT_EQ(img8.at<std::uint8_t>(i, i), 255);
        EXPECT_EQ(img8.at<std::uint8_t>(i, 9 - i), 127);
    }

    // Convert to 16-bpc
    auto img16 = Image::Convert(img, Depth::U16);
    for (const auto i : range(10)) {
        EXPECT_EQ(img16.at<std::uint16_t>(i, i), 65535);
        EXPECT_EQ(img16.at<std::uint16_t>(i, 9 - i), 32767);
    }
}

TEST(Image, Gamma)
{
    // Construct image and expected array
    Image img(1, 11, 1, Depth::F32);
    std::array<float, 11> expected;

    // Fill image and expected array
    float gamma{2.F};
    for (const auto x : range(11)) {
        // Assign to image
        img.at<float>(0, x) = 0.1F * float(x);
        // Calculate expected
        expected.at(x) = std::pow(0.1F * float(x), 1.F / gamma);
    }

    // Apply gamma correction
    auto gammaImg = Image::Gamma(img, 2.F);
    for (const auto x : range(11)) {
        EXPECT_FLOAT_EQ(gammaImg.at<float>(0, x), expected.at(x));
    }
}