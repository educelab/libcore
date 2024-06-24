# Check for float versions of std::from_chars
# See issue #1
set(code [[
    #include <charconv>

    int main() {
        std::string val{"5.0"};
        float result;
        std::from_chars(val.data(), val.data() + val.size(), result);
        return 0;
    }
]])
check_cxx_source_compiles("${code}" CXX_CHARCONV_FP_FROM_CHARS)
if(CXX_CHARCONV_FP_FROM_CHARS)
    message(STATUS "Float implementation for to_numeric: std::from_chars")
else()
    message(STATUS "Float implementation for to_numeric: std::sto[f|d|ld]")
    set(EDUCE_CORE_NEED_TO_NUMERIC_FP TRUE CACHE BOOL "Float implementations for to_numeric will use std::sto[f|d|ld]")
    add_compile_definitions(EDUCE_CORE_NEED_TO_NUMERIC_FP)
endif()