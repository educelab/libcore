#pragma once

/** @file */

#include <cstdint>
#include <vector>

namespace educelab
{

/** @brief Image bit depth */
enum class Depth {
    None, /** Unset or unspecified */
    U8,   /** Unsigned 8-bit integer */
    U16,  /** Unsigned 16-bit integer */
    F32   /** 32-bit float */
};

/** @brief Class for storing image data */
class Image
{
public:
    /**
     * @brief Default constructor
     *
     * Initializes an empty image.
     */
    Image() = default;

    /**
     * @brief Construct a new image of given height, width, and channels
     *
     * All pixels are initialized to 0 in all channels.
     */
    Image(std::size_t height, std::size_t width, std::size_t cns, Depth type);

    /** @brief Image width/columns */
    [[nodiscard]] auto width() const -> std::size_t;

    /** @brief Image height/rows */
    [[nodiscard]] auto height() const -> std::size_t;

    /** @brief Image aspect ratio */
    [[nodiscard]] auto aspect() const -> float;

    /** @brief Number of channels in the image */
    [[nodiscard]] auto channels() const -> std::size_t;

    /** @brief Fundamental type of each pixel element */
    [[nodiscard]] auto type() const -> Depth;

    /** @brief Whether image is empty */
    [[nodiscard]] auto empty() const -> bool;

    /** @brief Get the size of the image in bytes */
    [[nodiscard]] auto size() const -> std::size_t;

    /** @copydoc empty() */
    explicit operator bool() const noexcept;

    /** @brief Reset image */
    void clear();

    /** @brief Access pixel at coordinate (x,y) */
    template <typename T>
    auto at(std::size_t y, std::size_t x) -> T&;

    /** @copydoc at(std::size_t y, std::size_t x) */
    template <typename T>
    auto at(std::size_t y, std::size_t x) const -> const T&;

    /**
     * @brief Convert an Image to one of the specified bit depth
     *
     * When converting between integer types or to a floating point image,
     * intensities are scaled using the min/max ranges of the fundamental types,
     * with \f$[0, 1]\f$ being the output range for a float image. When
     * converting from a floating point image, the \f$[0, 1]\f$ range is scaled
     * to the range of the fundamental output type. Values which would fall
     * outside of the output range after scaling are clamped.
     */
    static auto Convert(const Image& i, Depth type) -> Image;

    /**
     * @brief Apply gamma correction to an image
     *
     * Applies gamma correction to each pixel in the image:
     * \f$ v_{out} = v_{in}^{1/\gamma} \f$
     */
    static auto Gamma(const Image& i, float gamma = 2.F) -> Image;

    /**
     * @copybrief Convert(const Image&, Depth)
     *
     * This is a convenience member function for calling Convert().
     */
    [[nodiscard]] auto convert(Depth type) const -> Image;

    /** @brief Returns a pointer to the underlying storage */
    [[nodiscard]] auto data() noexcept -> std::byte*;

    /** @copydoc data() */
    [[nodiscard]] auto data() const noexcept -> const std::byte*;

private:
    /** %Image height */
    std::size_t h_{0};
    /** %Image width */
    std::size_t w_{0};
    /** Number of channels */
    std::size_t cns_{0};
    /** Number of bytes between each element in a pixel */
    std::size_t stride_{0};
    /** Pixel element bit depth */
    Depth type_{Depth::None};
    /** Unravel a 2D index to a 1D position in the storage array */
    [[nodiscard]] auto unravel_(std::size_t y, std::size_t x) const
        -> std::size_t;
    /** Storage array */
    std::vector<std::byte> data_;
};

template <typename T>
auto Image::at(std::size_t y, std::size_t x) -> T&
{
    return reinterpret_cast<T&>(data_.at(unravel_(y, x)));
}

template <typename T>
auto Image::at(std::size_t y, std::size_t x) const -> const T&
{
    return reinterpret_cast<const T&>(data_.at(unravel_(y, x)));
}

}  // namespace educelab