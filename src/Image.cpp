#include "educelab/core/types/Image.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

using namespace educelab;

namespace
{
// Conversion constants
constexpr float max_u8{std::numeric_limits<std::uint8_t>::max()};
constexpr float max_u16{std::numeric_limits<std::uint16_t>::max()};
constexpr float u8_to_u16{max_u16 / max_u8};
constexpr float u8_to_f32{1.F / max_u8};
constexpr float u16_to_u8{max_u8 / max_u16};
constexpr float u16_to_f32{1.F / max_u16};

auto size_of(const Depth d) -> std::size_t
{
    switch (d) {
        case Depth::None:
            return 0;
        case Depth::U8:
            return 1;
        case Depth::U16:
            return 2;
        case Depth::F32:
            return 4;
    }
    return 0;
}

template <typename InType, typename OutType>
void depth_cast(const std::byte* in, std::byte* out);

template <>
void depth_cast<std::uint8_t, std::uint16_t>(
    const std::byte* in, std::byte* out)
{
    std::uint8_t tmpI{0};
    std::memcpy(&tmpI, in, sizeof(std::uint8_t));
    const auto f = static_cast<float>(tmpI) * u8_to_u16;
    const auto tmpO =
        static_cast<std::uint16_t>(std::max(std::min(f, max_u16), 0.F));
    std::memcpy(out, &tmpO, sizeof(std::uint16_t));
}

template <>
void depth_cast<std::uint8_t, float>(const std::byte* in, std::byte* out)
{
    std::uint8_t tmpI{0};
    std::memcpy(&tmpI, in, sizeof(std::uint8_t));
    const auto tmpO = static_cast<float>(tmpI) * u8_to_f32;
    std::memcpy(out, &tmpO, sizeof(float));
}

template <>
void depth_cast<std::uint16_t, std::uint8_t>(
    const std::byte* in, std::byte* out)
{
    std::uint16_t tmpI{0};
    std::memcpy(&tmpI, in, sizeof(std::uint16_t));
    const auto f = static_cast<float>(tmpI) * u16_to_u8;
    const auto tmpO =
        static_cast<std::uint8_t>(std::max(std::min(f, max_u8), 0.F));
    std::memcpy(out, &tmpO, sizeof(std::uint8_t));
}

template <>
void depth_cast<std::uint16_t, float>(const std::byte* in, std::byte* out)
{
    std::uint16_t tmpI{0};
    std::memcpy(&tmpI, in, sizeof(std::uint16_t));
    const auto tmpO = static_cast<float>(tmpI) * u16_to_f32;
    std::memcpy(out, &tmpO, sizeof(float));
}

template <>
void depth_cast<float, std::uint8_t>(const std::byte* in, std::byte* out)
{
    float tmpI{0};
    std::memcpy(&tmpI, in, sizeof(float));
    const auto tmpO = static_cast<std::uint8_t>(
        std::max(std::min(tmpI * max_u8, max_u8), 0.F));
    std::memcpy(out, &tmpO, sizeof(std::uint8_t));
}

template <>
void depth_cast<float, std::uint16_t>(const std::byte* in, std::byte* out)
{
    float tmpI{0};
    std::memcpy(&tmpI, in, sizeof(float));
    const auto tmpO = static_cast<std::uint16_t>(
        std::max(std::min(tmpI * max_u16, max_u16), 0.F));
    std::memcpy(out, &tmpO, sizeof(std::uint16_t));
}

template <typename TIn, typename TOut>
void pixel_cast(const std::byte* in, std::byte* out, const std::size_t cns)
{
    for (std::size_t idx{0}; idx < cns; idx++) {
        const auto i_idx = idx * sizeof(TIn);
        const auto o_idx = idx * sizeof(TOut);
        depth_cast<TIn, TOut>(&in[i_idx], &out[o_idx]);
    }
}

void convert_pixel(
    const std::byte* in,
    const Depth inType,
    std::byte* out,
    const Depth outType,
    const std::size_t cns)
{

    // 8bpc -> 16bpc
    if (inType == Depth::U8 and outType == Depth::U16) {
        pixel_cast<std::uint8_t, std::uint16_t>(in, out, cns);
    }

    // 8bpc -> float
    else if (inType == Depth::U8 and outType == Depth::F32) {
        pixel_cast<std::uint8_t, float>(in, out, cns);
    }

    // 16bpc -> 8bpc
    else if (inType == Depth::U16 and outType == Depth::U8) {
        pixel_cast<std::uint16_t, std::uint8_t>(in, out, cns);
    }

    // 16bpc -> float
    else if (inType == Depth::U16 and outType == Depth::F32) {
        pixel_cast<std::uint16_t, float>(in, out, cns);
    }

    // float -> 8bpc
    else if (inType == Depth::F32 and outType == Depth::U8) {
        pixel_cast<float, std::uint8_t>(in, out, cns);
    }

    // float -> 16bpc
    else if (inType == Depth::F32 and outType == Depth::U16) {
        pixel_cast<float, std::uint16_t>(in, out, cns);
    }

    // error
    else {
        throw std::runtime_error("Conversion not supported.");
    }
}
}  // namespace

Image::Image(
    const std::size_t height,
    const std::size_t width,
    const std::size_t cns,
    const Depth type)
    : h_{height}
    , w_{width}
    , cns_{cns}
    , stride_{size_of(type)}
    , type_{type}
    , data_(h_ * w_ * cns_ * stride_, std::byte{0})
{
    assert(h_ > 0 and w_ > 0 and cns_ > 0);
}

auto Image::width() const -> std::size_t { return w_; }

auto Image::height() const -> std::size_t { return h_; }

auto Image::aspect() const -> float
{
    return (h_ == 0) ? 0 : static_cast<float>(w_) / static_cast<float>(h_);
}

auto Image::channels() const -> std::size_t { return cns_; }

auto Image::type() const -> Depth { return type_; }

auto Image::empty() const -> bool { return data_.empty(); }

auto Image::size() const -> std::size_t { return data_.size(); }

Image::operator bool() const noexcept { return not empty(); }

void Image::clear()
{
    h_ = 0;
    w_ = 0;
    cns_ = 0;
    stride_ = 0;
    type_ = Depth::None;
    data_.clear();
}

auto Image::Convert(const Image& i, const Depth type) -> Image
{
    // Return if image is already of requested type
    if (i.type() == type) {
        return i;
    }

    // Setup output image
    Image result(i.h_, i.w_, i.cns_, type);

    // Iterate over original image data
    for (std::size_t y{0}; y < i.h_; y++) {
        for (std::size_t x{0}; x < i.w_; x++) {
            const auto i_idx = i.unravel_(y, x);
            const auto r_idx = result.unravel_(y, x);
            convert_pixel(
                &i.data_[i_idx], i.type(), &result.data_[r_idx], type, i.cns_);
        }
    }

    return result;
}

auto Image::Gamma(const Image& i, const float gamma) -> Image
{
    auto result = Convert(i, Depth::F32);
    for (std::size_t y{0}; y < result.h_; y++) {
        for (std::size_t x{0}; x < result.w_; x++) {
            const auto idx = result.unravel_(y, x);
            for (std::size_t c{0}; c < result.cns_; c++) {
                const auto o = c * sizeof(float);
                auto* v = reinterpret_cast<float*>(&result.data_[idx + o]);
                *v = std::pow(*v, 1.F / gamma);
            }
        }
    }
    result = Convert(result, i.type());
    return result;
}

auto Image::convert(const Depth type) const -> Image
{
    return Convert(*this, type);
}

auto Image::data() noexcept -> std::byte* { return data_.data(); }

auto Image::data() const noexcept -> const std::byte* { return data_.data(); }

auto Image::unravel_(const std::size_t y, const std::size_t x) const
    -> std::size_t
{
    return (y % h_) * w_ * cns_ * stride_ + x * cns_ * stride_;
}
