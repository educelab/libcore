#include "educelab/core/io/ImageIO.hpp"

#include "educelab/core/types/Vec.hpp"
#include "educelab/core/utils/Filesystem.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;
namespace el = educelab;
namespace fs = std::filesystem;

// Write PPM image
static void ppm_write(const fs::path& path, const Image& image)
{
    // PPM only supports 8bpc, RGB
    if (image.channels() != 3) {
        throw std::runtime_error(
            "Unsupported number of channels: " +
            std::to_string(image.channels()));
    }
    auto tmp = image.convert(Depth::U8);

    // Open output file
    std::ofstream file(path);
    if (not file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    // PPM Header
    file << "P3\n";
    file << tmp.width() << ' ' << tmp.height() << '\n';
    file << "255\n";

    // Pixels
    for (const auto [y, x] : range2D(tmp.height(), tmp.width())) {
        auto p = tmp.at<Vec3b>(y, x);
        file << static_cast<unsigned int>(p[0]) << ' ';
        file << static_cast<unsigned int>(p[1]) << ' ';
        file << static_cast<unsigned int>(p[2]) << '\n';
    }

    file.close();
}

void el::write_image(const fs::path& path, const Image& image)
{
    if (is_file_type(path, "ppm")) {
        ppm_write(path, image);
    } else {
        auto ext = path.extension().string();
        throw std::invalid_argument("Unsupported file type: " + ext);
    }
}
