include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    EduceLabCoreConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
)
configure_file(cmake/EduceLabCoreConfig.cmake.in EduceLabCoreConfig.cmake @ONLY)

install(
    EXPORT EduceLabCoreTargets
    FILE EduceLabCoreTargets.cmake
    NAMESPACE educelab::
    DESTINATION lib/cmake/EduceLab
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/EduceLabCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/EduceLabCoreConfigVersion.cmake"
    DESTINATION lib/cmake/EduceLab
)
