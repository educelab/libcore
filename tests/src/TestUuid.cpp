#include <gtest/gtest.h>

#include "educelab/core/types/Uuid.hpp"

using namespace educelab;

TEST(Uuid, Default)
{
    Uuid uuid;
    EXPECT_TRUE(uuid.is_nil());
}

TEST(Uuid, StringFunctions)
{
    // Construct a uuid from a known string
    std::string uuidStr{"2d243fb2-91c8-48ef-beb7-fb60966b2316"};
    auto uuid = Uuid::FromString(uuidStr);
    EXPECT_FALSE(uuid.is_nil());

    // Get the string back from the new Uuid
    auto str = uuid.string();
    EXPECT_EQ(str, uuidStr);

    // Construct a new Uuid from the provided string
    auto uuidClone = Uuid::FromString(str);
    EXPECT_FALSE(uuidClone.is_nil());
    EXPECT_EQ(uuid, uuidClone);
}