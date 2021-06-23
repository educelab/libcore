#pragma once

/** @file */

#include <regex>
#include <string>
#include <variant>

#include "educelab/core/types/Vec.hpp"

namespace educelab
{

namespace detail
{
/** Regular expression for recognizing hexadecimal color codes */
const std::regex REGEX_HEX_COLOR{"^#([A-Fa-f0-9]{3}|[A-Fa-f0-9]{6})$"};
}  // namespace detail

/**
 * @brief A single class for storing color values and converting them to
 * alternative color formats
 *
 * Usage example:
 * ```{.cpp}
 * Color c;
 * std::cout << c.type() << "\n";                // "None"
 *
 * c = Color::U8C1{128};
 * std::cout << c.type_name() << "\n";           // "U8C1"
 * std::cout << c.value<Color::U8C1>() << "\n";  // "128"
 *
 * c = Color::U8C3{255, 0, 0};
 * std::cout << c.type_name() << "\n";           // "U8C3"
 * std::cout << c.value<Color::U8C3>() << "\n";  // "[255, 0, 0]"
 * ```
 */
class Color
{
public:
    /** Color types enum */
    enum class Type : std::size_t {
        /** No assigned value */
        None = 0,
        /** 8-bit grayscale color */
        U8C1,
        /** 8-bit RGB color */
        U8C3,
        /** 8-bit RGBA color */
        U8C4,
        /** 16-bit grayscale color */
        U16C1,
        /** 16-bit RGB color */
        U16C3,
        /** 16-bit RGBA color */
        U16C4,
        /** 32-bit float grayscale color */
        F32C1,
        /** 32-bit float RGB color */
        F32C3,
        /** 32-bit float RGBA color */
        F32C4,
        /** Hexadecimal RGB color string of 3 or 6 digits (`#0a3` or `#00aa33`)
         */
        HexCode
    };

    /** 8-bit grayscale color value type */
    using U8C1 = uint8_t;
    /** 8-bit RGB color value type */
    using U8C3 = Vec<uint8_t, 3>;
    /** 8-bit RGBA color value type */
    using U8C4 = Vec<uint8_t, 4>;
    /** 16-bit grayscale color value type */
    using U16C1 = uint16_t;
    /** 16-bit RGB color value type */
    using U16C3 = Vec<uint16_t, 3>;
    /** 16-bit RGBA color value type */
    using U16C4 = Vec<uint16_t, 4>;
    /** 32-bit float grayscale color value type */
    using F32C1 = float;
    /** 32-bit float RGB color value type */
    using F32C3 = Vec<float, 3>;
    /** 32-bit float RGBA color value type */
    using F32C4 = Vec<float, 4>;
    /** Hexadecimal RGB color value type */
    using HexCode = std::string;

    /** @brief Default constructor */
    Color() = default;

    /** @brief Construct from value */
    template <typename T>
    explicit Color(const T& v) : val_{v}
    {
    }

    /** @brief Construct from hexadecimal CString */
    explicit Color(const char* str)
    {
        if (not std::regex_match(str, detail::REGEX_HEX_COLOR)) {
            throw std::invalid_argument(
                "CString not a hex color code: " + std::string(str));
        }
        val_ = str;
    }

    /** @brief Construct from hexadecimal std::string */
    explicit Color(const std::string& str)
    {
        if (not std::regex_match(str, detail::REGEX_HEX_COLOR)) {
            throw std::invalid_argument("String not a hex color code: " + str);
        }
        val_ = str;
    }

    /** @brief Assign a value */
    template <typename T>
    auto operator=(const T& v) -> Color&
    {
        val_ = v;
        return *this;
    }

    /** @brief Assign a hexadecimal CString value */
    auto operator=(const char* str) -> Color&
    {
        if (not std::regex_match(str, detail::REGEX_HEX_COLOR)) {
            throw std::invalid_argument(
                "CString not a hex color code: " + std::string(str));
        }
        val_ = str;
        return *this;
    }

    /** @brief Assign a hexadecimal std::string value */
    auto operator=(const std::string& str) -> Color&
    {
        if (not std::regex_match(str, detail::REGEX_HEX_COLOR)) {
            throw std::invalid_argument("String not a hex color code: " + str);
        }
        val_ = str;
        return *this;
    }

    /** @brief Equality comparison operator */
    auto operator==(const Color& rhs) const -> bool { return val_ == rhs.val_; }

    /** @brief Inequality comparison operator */
    auto operator!=(const Color& rhs) const -> bool { return val_ != rhs.val_; }

    /** @brief Get the stored color type */
    [[nodiscard]] auto type() const -> Type
    {
        return static_cast<Type>(val_.index());
    }

    /** @brief Get the human-readable color type */
    [[nodiscard]] auto type_name() const -> std::string
    {
        switch (type()) {
            case Type::None:
                return "None";
            case Type::U8C1:
                return "U8C1";
            case Type::U8C3:
                return "U8C3";
            case Type::U8C4:
                return "U8C4";
            case Type::U16C1:
                return "U16C1";
            case Type::U16C3:
                return "U16C3";
            case Type::U16C4:
                return "U16C4";
            case Type::F32C1:
                return "F32C1";
            case Type::F32C3:
                return "F32C3";
            case Type::F32C4:
                return "F32C4";
            case Type::HexCode:
                return "HexCode";
        }
    }

    /** @brief Check if the Color object has a stored value */
    [[nodiscard]] auto has_value() const -> bool { return val_.index() > 0; }

    /**
     * @brief Get the stored color value in the format of the requested color
     * type
     */
    template <typename T>
    [[nodiscard]] auto value() const -> T
    {
        // Already of the requested type
        if (std::holds_alternative<T>(val_)) {
            return std::get<T>(val_);
        } else {
            // TODO: Do convert to requested type
            throw std::bad_variant_access();
        }
    }

    /** @brief Clear the stored value */
    auto clear() -> void { val_ = std::monostate{}; }

private:
    /** Container type for storing all color values */
    using Container = std::variant<
        std::monostate,
        U8C1,
        U8C3,
        U8C4,
        U16C1,
        U16C3,
        U16C4,
        F32C1,
        F32C3,
        F32C4,
        HexCode>;
    /** Color value */
    Container val_{};
};
}  // namespace educelab
