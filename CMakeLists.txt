cmake_minimum_required(VERSION 3.4.0)

if(${CMAKE_VERSION} VERSION_LESS 3.11)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.11)
endif()

message(STATUS "CMAKE_VERSION: ${CMAKE_VERSION}")

# Use static MSVC runtime.
# This should be done before the project command.
function (override var file)
  if (EXISTS "${file}")
    set(${var} ${file} PARENT_SCOPE)
  endif ()
endfunction ()
override(CMAKE_USER_MAKE_RULES_OVERRIDE
  ${CMAKE_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
override(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
  ${CMAKE_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)


project(TABLES VERSION 0.0.1 LANGUAGES CXX C)


if (MSVC)
  # Disable useless MSVC warnings suggesting nonportable "secure" alternatives.
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# CPack
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_VERSION ${ALIC_VERSION})
set(CPACK_PACKAGE_FILE_NAME tables)
include(CPack)

# CTest
include(CTest)
enable_testing()

# c++11 support
set (CMAKE_CXX_STANDARD 11)

add_subdirectory(thirdparty/asl)
add_subdirectory(src)
add_subdirectory(test/tables)

