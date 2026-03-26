# Check floating-point support in std::from_chars and std::to_chars,
# probed independently for float, double, and long double.
#
# Both were added in C++17 but compiler/platform support arrived at different
# times and varies per type. Apple Clang / libc++ added float/double support
# in macOS 13.3; long double availability differs further.
#
# For each (function, type) combination a separate compile-time probe is run.
# When unavailable, a per-type preprocessor definition is set so String.hpp
# can activate the appropriate fallback specialisation without over-disabling
# types that are actually supported.
#
# Definitions emitted (only when the corresponding function+type is absent):
#   EDUCE_CORE_NEED_FROM_CHARS_FLOAT
#   EDUCE_CORE_NEED_FROM_CHARS_DOUBLE
#   EDUCE_CORE_NEED_FROM_CHARS_LONG_DOUBLE
#   EDUCE_CORE_NEED_TO_CHARS_FLOAT
#   EDUCE_CORE_NEED_TO_CHARS_DOUBLE
#   EDUCE_CORE_NEED_TO_CHARS_LONG_DOUBLE

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

cmake_push_check_state(RESET)

if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/std:c++17")
else()
    set(CMAKE_REQUIRED_FLAGS "-std=c++17")
endif()

# ---------------------------------------------------------------------------
# Helper macro: probe one (function, type) combination.
#
# Usage:
#   _charconv_probe(FROM_CHARS float  "float result; std::from_chars(s,s+3,result);"  NEED_FROM_CHARS_FLOAT)
# ---------------------------------------------------------------------------
macro(_charconv_probe _direction _typename _snippet _defname)
    set(_probe_code "
        #include <charconv>
        #include <array>
        int main() {
            const char src[] = \"5.0\";
            ${_snippet}
            return 0;
        }
    ")
    check_cxx_source_compiles("${_probe_code}" _CXX_CHARCONV_${_defname})
    if(NOT _CXX_CHARCONV_${_defname})
        add_compile_definitions(EDUCE_CORE_${_defname})
    endif()
endmacro()

# --- from_chars ---------------------------------------------------------------
_charconv_probe(
    "from_chars" "float"
    "float r{}; std::from_chars(src, src+3, r);"
    NEED_FROM_CHARS_FLOAT)

_charconv_probe(
    "from_chars" "double"
    "double r{}; std::from_chars(src, src+3, r);"
    NEED_FROM_CHARS_DOUBLE)

_charconv_probe(
    "from_chars" "long double"
    "long double r{}; std::from_chars(src, src+3, r);"
    NEED_FROM_CHARS_LONG_DOUBLE)

# --- to_chars -----------------------------------------------------------------
_charconv_probe(
    "to_chars" "float"
    "std::array<char,32> buf{}; float v{5.f}; std::to_chars(buf.data(), buf.data()+buf.size(), v);"
    NEED_TO_CHARS_FLOAT)

_charconv_probe(
    "to_chars" "double"
    "std::array<char,32> buf{}; double v{5.0}; std::to_chars(buf.data(), buf.data()+buf.size(), v);"
    NEED_TO_CHARS_DOUBLE)

_charconv_probe(
    "to_chars" "long double"
    "std::array<char,32> buf{}; long double v{5.0L}; std::to_chars(buf.data(), buf.data()+buf.size(), v);"
    NEED_TO_CHARS_LONG_DOUBLE)

cmake_pop_check_state()
