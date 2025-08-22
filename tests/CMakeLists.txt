## Google Test ##
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL OFF FORCE)
FetchContent_MakeAvailable(googletest)

## Build the tests ##
set(tests
    src/TestCaching.cpp
    src/TestColor.cpp
    src/TestFilesystem.cpp
    src/TestFlags.cpp
    src/TestImage.cpp
    src/TestIteration.cpp
    src/TestLinearAlgebra.cpp
    src/TestMat.cpp
    src/TestMath.cpp
    src/TestMesh.cpp
    src/TestSignals.cpp
    src/TestString.cpp
    src/TestUuid.cpp
    src/TestVec.cpp
    src/TestVersion.cpp
)

foreach(src ${tests})
    get_filename_component(filename ${src} NAME_WE)
    set(testname educelab_core_${filename})
    add_executable(${testname} ${src})
    target_link_libraries(${testname}
        educelab::core
        gtest_main
        gmock_main
    )
    add_test(
        NAME ${testname}
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMAND ${testname}
    )
endforeach()