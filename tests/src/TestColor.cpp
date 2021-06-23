#include <gtest/gtest.h>

#include "educelab/core/types/Color.hpp"

using namespace educelab;

TEST(Color, Assignment)
{
    Color color;
    EXPECT_FALSE(color.has_value());

    Color::U8C1 gray8{255};
    color = gray8;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U8C1);
    EXPECT_EQ(color.value<Color::U8C1>(), gray8);

    Color::U8C3 rgb8{255, 0, 0};
    color = rgb8;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U8C3);
    EXPECT_EQ(color.value<Color::U8C3>(), rgb8);

    Color::U8C4 rgba8{255, 0, 0, 255};
    color = rgba8;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U8C4);
    EXPECT_EQ(color.value<Color::U8C4>(), rgba8);

    Color::U16C1 gray16{65535};
    color = gray16;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U16C1);
    EXPECT_EQ(color.value<Color::U16C1>(), gray16);

    Color::U16C3 rgb16{65535, 0, 0};
    color = rgb16;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U16C3);
    EXPECT_EQ(color.value<Color::U16C3>(), rgb16);

    Color::U16C4 rgba16{65535, 0, 0, 65535};
    color = rgba16;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::U16C4);
    EXPECT_EQ(color.value<Color::U16C4>(), rgba16);

    Color::F32C1 gray32{1};
    color = gray32;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::F32C1);
    EXPECT_EQ(color.value<Color::F32C1>(), gray32);

    Color::F32C3 rgb32{1, 0, 0};
    color = rgb32;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::F32C3);
    EXPECT_EQ(color.value<Color::F32C3>(), rgb32);

    Color::F32C4 rgba32{1, 0, 0, 1};
    color = rgba32;
    EXPECT_TRUE(color.has_value());
    EXPECT_EQ(color.type(), Color::Type::F32C4);
    EXPECT_EQ(color.value<Color::F32C4>(), rgba32);

    for (const auto& hex : {"#f0a", "#ff00aa"}) {
        color = hex;
        EXPECT_TRUE(color.has_value());
        EXPECT_EQ(color.type(), Color::Type::HexCode);
        EXPECT_EQ(color.value<Color::HexCode>(), hex);
    }
    EXPECT_THROW(color = "#badhex", std::invalid_argument);

    color.clear();
    EXPECT_FALSE(color.has_value());
}
