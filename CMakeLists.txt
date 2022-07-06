cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(EduceLabCore VERSION 0.1)

# Setup project directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Modules
include(FetchContent)
include(CheckCXXSourceCompiles)
include(CMakeDependentOption)

# Get Git hash
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
if(GIT_SHA1)
    string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1_SHORT)
else()
    set(GIT_SHA1 Untracked)
    set(GIT_SHA1_SHORT Untracked)
endif()

# Extra compile definitions
include(CheckToNumericFP)

# Build library
set(public_hdrs
    include/educelab/core.hpp
    include/educelab/core/Version.hpp
    include/educelab/core/io/ImageIO.hpp
    include/educelab/core/types/Color.hpp
    include/educelab/core/types/Image.hpp
    include/educelab/core/types/Mat.hpp
    include/educelab/core/types/Mesh.hpp
    include/educelab/core/types/Signals.hpp
    include/educelab/core/types/Uuid.hpp
    include/educelab/core/types/Vec.hpp
    include/educelab/core/utils/Caching.hpp
    include/educelab/core/utils/Filesystem.hpp
    include/educelab/core/utils/Iteration.hpp
    include/educelab/core/utils/LinearAlgebra.hpp
    include/educelab/core/utils/Math.hpp
    include/educelab/core/utils/String.hpp
)

configure_file(src/Version.cpp.in Version.cpp)
set(srcs
    ${public_hdrs}
    ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp
    src/Image.cpp
    src/ImageIO.cpp
    src/Uuid.cpp
)

add_library(core ${srcs})
add_library(educelab::core ALIAS core)
target_include_directories(core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(core PUBLIC cxx_std_17)
set_target_properties(core
    PROPERTIES
        PUBLIC_HEADER "${public_hdrs}"
)
install(
    TARGETS core
    EXPORT EduceLabCoreTargets
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    INCLUDES DESTINATION "include/educelab/core"
    PUBLIC_HEADER DESTINATION "include/educelab/core"
)

# Docs
find_package(Doxygen OPTIONAL_COMPONENTS dot)
CMAKE_DEPENDENT_OPTION(EDUCE_CORE_BUILD_DOCS "Build Doxygen documentation" off "DOXYGEN_FOUND" off)
if(EDUCE_CORE_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Examples
option(EDUCE_CORE_BUILD_EXAMPLES "Compile EduceLab Core example applications" off)
if(EDUCE_CORE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
option(EDUCE_CORE_BUILD_TESTS "Compile EduceLab Core unit tests" off)
if(EDUCE_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
include(InstallProject)
