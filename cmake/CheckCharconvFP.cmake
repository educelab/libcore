# Check floating-point support in std::from_chars and std::to_chars separately.
#
# Both were added in C++17 but compiler/platform support arrived at different
# times. Apple Clang and older GCC shipped integer charconv first; floating-
# point support followed later and is gated on the runtime library version
# (e.g. macOS 13.3+ for libc++).
#
# Two probes are run and reported independently. Only the from_chars result
# drives EDUCE_CORE_NEED_CHARCONV_FP because that is the only function with a
# compile-time fallback in String.hpp (to_numeric). to_string_view uses
# to_chars unconditionally and requires a platform where it is available.

# --- from_chars (float) ------------------------------------------------------
set(_from_chars_code [[
    #include <charconv>

    int main() {
        const char src[] = "5.0";
        float result;
        std::from_chars(src, src + 3, result);
        return 0;
    }
]])
check_cxx_source_compiles("${_from_chars_code}" CXX_CHARCONV_FP_FROM_CHARS)

if(CXX_CHARCONV_FP_FROM_CHARS)
    message(STATUS "Float from_chars: std::from_chars (native)")
else()
    message(STATUS "Float from_chars: std::sto[f|d|ld] (fallback)")
    set(EDUCE_CORE_NEED_CHARCONV_FP TRUE CACHE BOOL
        "std::from_chars unavailable for float; to_numeric uses std::sto fallbacks")
    add_compile_definitions(EDUCE_CORE_NEED_CHARCONV_FP)
endif()

# --- to_chars (float) --------------------------------------------------------
set(_to_chars_code [[
    #include <charconv>
    #include <array>

    int main() {
        std::array<char, 32> buf{};
        float val{5.0f};
        std::to_chars(buf.data(), buf.data() + buf.size(), val);
        return 0;
    }
]])
check_cxx_source_compiles("${_to_chars_code}" CXX_CHARCONV_FP_TO_CHARS)

if(CXX_CHARCONV_FP_TO_CHARS)
    message(STATUS "Float to_chars:   std::to_chars (native)")
else()
    message(STATUS "Float to_chars:   unavailable — to_string_view requires a platform where std::to_chars supports float")
endif()
