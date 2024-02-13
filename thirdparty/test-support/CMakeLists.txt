# Helper library from mp
find_package(Threads)
include(CheckCXXCompilerFlag)
check_cxx_source_compiles("
    #include <initializer_list>
    int main() {}" MP_INITIALIZER_LISTS)
if (NOT MP_VARIADIC_TEMPLATES OR NOT MP_INITIALIZER_LISTS)
    add_definitions(-DGTEST_LANG_CXX11=0)
endif ()

# Workaround GTest bug https://github.com/google/googletest/issues/705.
check_cxx_compiler_flag(
  -fno-delete-null-pointer-checks HAVE_FNO_DELETE_NULL_POINTER_CHECKS)


set(MP_TEST_SOURCES 
      ${CMAKE_CURRENT_SOURCE_DIR}/function.h
      ${CMAKE_CURRENT_SOURCE_DIR}/util.h
      ${CMAKE_CURRENT_SOURCE_DIR}/function.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/format.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/util.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/test-main.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/../gmock/gmock-gtest-all.cc)

set(target_name test-support)
add_library(${target_name} STATIC ${MP_TEST_SOURCES})
target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT} asl)
target_include_directories(${target_name} PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (HAVE_FNO_DELETE_NULL_POINTER_CHECKS)
  target_compile_options(${target_name} PUBLIC -fno-delete-null-pointer-checks)
endif ()
# Workaround a bug in implementation of variadic templates in MSVC11.
if (MSVC)
  target_compile_definitions(${target_name} PUBLIC _VARIADIC_MAX=10)
endif()