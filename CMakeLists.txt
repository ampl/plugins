cmake_minimum_required(VERSION 3.4.0)

if(${CMAKE_VERSION} VERSION_LESS 3.11)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.11)
endif()

project(TABLES VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# CPack
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_VERSION ${ALIC_VERSION})
set(CPACK_PACKAGE_FILE_NAME tables)
include(CPack)

# CTest
include(CTest)
enable_testing()

set(AMPL_LIBRARY_DIR .)

add_subdirectory(src)

