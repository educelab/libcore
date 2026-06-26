cmake_minimum_required(VERSION 3.15...4.3 FATAL_ERROR)
project(EduceLabCore VERSION 0.3.0)

# Setup project directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Modules
include(FetchContent)
include(CMakeDependentOption)
include(GNUInstallDirs)

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
include(CheckCharconvFP)

# Build library
set(public_hdrs
    include/educelab/core.hpp
    include/educelab/core/Version.hpp
    include/educelab/core/io/ImageIO.hpp
    include/educelab/core/io/MeshIO.hpp
    include/educelab/core/io/MeshIO_OBJ.hpp
    include/educelab/core/io/MeshIO_PLY.hpp
    include/educelab/core/types/Color.hpp
    include/educelab/core/types/Image.hpp
    include/educelab/core/types/Mat.hpp
    include/educelab/core/types/Mesh.hpp
    include/educelab/core/types/Signals.hpp
    include/educelab/core/types/UVMap.hpp
    include/educelab/core/types/Uuid.hpp
    include/educelab/core/types/Vec.hpp
    include/educelab/core/utils/Caching.hpp
    include/educelab/core/utils/Flags.hpp
    include/educelab/core/utils/MeshUtils.hpp
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
if(EDUCE_CORE_CHARCONV_DEFS)
    target_compile_definitions(core PUBLIC ${EDUCE_CORE_CHARCONV_DEFS})
endif()
install(
    TARGETS core
    EXPORT EduceLabCoreTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
# Install headers preserving their directory structure under include/educelab/core
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp"
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
