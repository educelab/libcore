#pragma once

/** @file */

#include <filesystem>
#include <regex>
#include <string_view>
#include <vector>

#include "educelab/core/utils/String.hpp"

namespace educelab
{

namespace detail
{
/** Compare file extension against list of extensions */
template <typename Extension, typename... ExtensionList>
auto extension_is_in_list(
    std::string_view ext, const Extension& first, const ExtensionList&... rest)
    -> bool
{
    if (ext == to_lower(first)) {
        return true;
    }

    if constexpr (sizeof...(rest) > 0) {
        return extension_is_in_list(ext, rest...);
    }

    return false;
}
}  // namespace detail

/**
 * @brief Returns true if `path` has a file extension in `exts`. Comparison is
 * case insensitive.
 */
template <typename... ExtensionList>
auto is_file_type(
    const std::filesystem::path& path, const ExtensionList&... exts) -> bool
{
    if (not path.has_extension()) {
        return false;
    }
    auto ext = to_lower(path.extension().string().substr(1));
    return detail::extension_is_in_list(ext, exts...);
}

}  // namespace educelab
