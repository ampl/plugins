# Add test-support library
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/test-support ${CMAKE_CURRENT_BINARY_DIR}/mptest)
addToTablesFolder(test test-support)

function(add_mp_test name)
  cmake_parse_arguments(add_mp_test "" "" LIBS ${ARGN})
  add_executable(${name} ${add_mp_test_UNPARSED_ARGUMENTS})
  target_compile_definitions(${name} PRIVATE
  AMPLGSL_DLL_NAME=)
  set_target_properties(${name} PROPERTIES OUTPUT_NAME ${name})
  if (MINGW)
    set_target_properties(${name} PROPERTIES
      LINK_FLAGS "-static-libgcc -static-libstdc++")
  endif ()
  if (NOT add_mp_test_LIBS)
    set(add_mp_test_LIBS asl)
  endif ()
  target_link_libraries(${name} test-support ${add_mp_test_LIBS})
  target_compile_definitions(${name} PRIVATE
    MP_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/test/data"
    AMPLTABL_DLL_NAME="$<TARGET_FILE:ampltabl>"
    FULLBIT_DLL_NAME="$<TARGET_FILE:fullbit>"
    TABLEPROXY_EXE_NAME="$<TARGET_FILE:tableproxy>"
    )
  add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
  addToTablesFolder(test ${name})
endfunction()

find_package(ODBC)
if (ODBC_FOUND)
  include_directories(${ODBC_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${ODBC_LIBRARIES})

  add_mp_test(odbc-test odbc-test.cc odbc.cc LIBS ${ODBC_LIBRARIES} asl2)

  include(CheckCXXSourceRuns)
  check_cxx_source_runs(
    "#include \"odbc.cc\"
     int main() {
       return odbc::Env().FindDriver(\"*.accdb\").empty() ? 1 : 0;
     }" HAVE_ACCESS_DRIVER)

  if (HAVE_ACCESS_DRIVER)
    add_mp_test(access-test access-test.cc odbc.cc
      LIBS ${ODBC_LIBRARIES})
  endif ()

  check_cxx_source_runs(
    "#include \"odbc.cc\"
     int main() {
       return odbc::Env().FindDriver(\"*.xlsx\").empty() ? 1 : 0;
     }" HAVE_EXCEL_DRIVER)

  if (HAVE_EXCEL_DRIVER)
    add_mp_test(excel-test excel-test.cc odbc.cc
      LIBS ${ODBC_LIBRARIES})
  endif ()

  check_cxx_source_runs(
    "#include \"odbc.cc\"
     int main() {
       return odbc::Env().FindDriver(\"*.csv\").empty() ? 1 : 0;
     }" HAVE_ODBC_TEXT_DRIVER)

  if (HAVE_ODBC_TEXT_DRIVER)
    add_mp_test(csv-test csv-test.cc odbc.cc LIBS ${ODBC_LIBRARIES})
  endif ()

  if (ENABLE_MYSQL_TESTS)
    check_cxx_source_runs(
      "#include \"odbc.cc\"
      int main() {
        return odbc::Env().FindDriver(\"mysql\").empty() ? 1 : 0;
      }" HAVE_MYSQL_DRIVER)

    if (HAVE_MYSQL_DRIVER)
      if (WIN32)
        set(MYSQL_TEST_LIBRARIES Ws2_32.lib)
      endif ()
      add_mp_test(mysql-test mysql-test.cc odbc.cc
        LIBS ${ODBC_LIBRARIES} ${MYSQL_TEST_LIBRARIES})
    endif ()
  endif ()
endif ()

if (TARGET ampltabl)
  add_mp_test(tableproxy-test tableproxy-test.cc LIBS asl)
  add_dependencies(tableproxy-test ampltabl)
endif ()
