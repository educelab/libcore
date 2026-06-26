include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    EduceLabCoreConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
)
configure_file(cmake/EduceLabCoreConfig.cmake.in EduceLabCoreConfig.cmake @ONLY)

# Package config must live in a directory matching find_package(EduceLabCore)'s
# search glob (<prefix>/<libdir>/cmake/EduceLabCore*), otherwise it is not found.
set(EduceLabCore_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/EduceLabCore")

install(
    EXPORT EduceLabCoreTargets
    FILE EduceLabCoreTargets.cmake
    NAMESPACE educelab::
    DESTINATION "${EduceLabCore_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/EduceLabCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/EduceLabCoreConfigVersion.cmake"
    DESTINATION "${EduceLabCore_INSTALL_CMAKEDIR}"
)
