#include <gtest/gtest.h>

#include "educelab/core/Version.hpp"

using namespace educelab::core;

TEST(Version, ProjectInfo)
{
    EXPECT_EQ(ProjectInfo::Name(), "EduceLab libcore");
    EXPECT_EQ(
        ProjectInfo::RepositoryURL(), "https://gitlab.com/educelab/libcore");
}
