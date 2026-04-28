# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.16)

project(orVocApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(orVocApp)
add_subdirectory(tests)

# ===== CPack configuration =====
set(CPACK_PACKAGE_NAME "orvocapp")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Personal vocabulary builder with English definitions and Arabic translations")
set(CPACK_PACKAGE_CONTACT "orfi")

# ----- DEB generator -----
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/postinst;${CMAKE_SOURCE_DIR}/packaging/postrm")

# ----- NSIS generator (Windows) -----
set(CPACK_NSIS_DISPLAY_NAME "orVocApp")
set(CPACK_NSIS_PACKAGE_NAME "orVocApp")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_MODIFY_PATH OFF)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/orVocApp/icon.ico")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\orVocApp\\\\orVocApp.lnk' '$INSTDIR\\\\bin\\\\orVocApp.exe' '' '$INSTDIR\\\\bin\\\\icon.ico' 0")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\orVocApp\\\\orVocApp.lnk'")

include(CPack)
