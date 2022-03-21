#pragma once

/** @file */

#include <filesystem>
#include <fstream>

#include "educelab/core/types/Image.hpp"

namespace educelab
{

/** @brief Write an Image to disk */
void write_image(const std::filesystem::path& path, const Image& image);

}  // namespace educelab