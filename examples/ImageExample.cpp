#include "educelab/core/io/ImageIO.hpp"
#include "educelab/core/types/Image.hpp"
#include "educelab/core/types/Vec.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

auto main() -> int
{
    // Construct an image
    Image image(600, 800, 3, Depth::F32);

    // Fill image with a color gradient
    for (const auto [y, x] : range2D(image.height(), image.width())) {
        // Create RGB values
        auto r = float(x) / float(image.width() - 1);
        auto g = float(y) / float(image.height() - 1);
        auto b = 0.25F;

        // Assign value to the correct pixel. Like its equivalent in OpenCV,
        // Image.at gives you access to the raw image buffer memory,
        // reinterpreted as the type you request. Make sure you know the data
        // type and channels when you access a pixel!
        image.at<Vec3f>(y, x) = {r, g, b};
    }

    // (Optional) Apply gamma correction
    image = Image::Gamma(image);

    // (Optional) Convert image to the correct format for output. write_image()
    // will automatically convert if the output format doesn't support the
    // given bit depth, but it's always good to control the conversion.
    image = image.convert(Depth::U8);

    // Write the image
    write_image("educelab_core_ImageExample.ppm", image);
}
