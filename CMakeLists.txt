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


project(plugins VERSION 0.0.1 LANGUAGES CXX C)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Test if I have a parent project, in which case put generated targets 
# in the tables folder on supported generators
get_directory_property(hasParent PARENT_DIRECTORY)

function(addToTablesFolder folder)
    if(hasParent)
        set(prefix tables/)
    else()
        set(prefix "")
    endif()
    add_to_folder("${prefix}${folder}" ${ARGN})
endfunction()

if (MSVC)
  # Disable useless MSVC warnings suggesting nonportable "secure" alternatives.
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# c++11 support
set (CMAKE_CXX_STANDARD 11)



# CTest
include(CTest)
enable_testing()

set(ASL_SKIP_INSTALL 1) # do not install ASL
add_subdirectory(thirdparty/asl)
add_subdirectory(src)
add_subdirectory(test/tables)
add_subdirectory(doc)

# CPack
if(MSVC)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VERSION ${ALIC_VERSION})
set(CPACK_PACKAGE_FILE_NAME plugins)
include(CPack)
