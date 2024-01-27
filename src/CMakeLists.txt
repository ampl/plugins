# Find programs
if (NOT AMPL_PROGRAM)
  find_program(AMPL_PROGRAM ampl)
endif ()
find_program(PYTHON_PROGRAM python)

include(CheckCCompilerFlag)
# Check if the compiler supports a flag. If it does, add it to target.
function (add_c_compiler_flags target)
  foreach (flag ${ARGN})
    string(REPLACE "-" "_" var ${flag})
    check_c_compiler_flag(${flag} ${var})
    if (${var})
      target_compile_options(${target} PRIVATE ${flag})
    endif ()
  endforeach ()
endfunction ()

set(AMPL_LIBRARY_DIR .)

# Adds a shared AMPL library which by convention doesn't have any prefix
# and has a suffix ".dll" on all platforms.
macro(add_ampl_library name)
  cmake_parse_arguments(add_ampl_library PRIVATE "COMPONENT" "" ${ARGN})
  add_library(${name} SHARED ${add_ampl_library_UNPARSED_ARGUMENTS})
  set_target_properties(${name} PROPERTIES PREFIX "")
  set_target_properties(${name} PROPERTIES SUFFIX ".dll")
  if (NOT add_ampl_library_PRIVATE)
    if (add_ampl_library_COMPONENT)
        set(PNAME ${add_ampl_library_COMPONENT})
    else()
        set(PNAME ${ALL_COMPONENT})
    endif()
    if(MSVC)
      install(TARGETS ${name} RUNTIME DESTINATION ${AMPL_LIBRARY_DIR} COMPONENT ${PNAME})
    else()
      install(TARGETS ${name} RUNTIME DESTINATION ${AMPL_LIBRARY_DIR} 
                              LIBRARY DESTINATION ${AMPL_LIBRARY_DIR} COMPONENT ${PNAME})
    endif()
  endif ()
endmacro()

# amplp libraries
add_subdirectory(amplp/examples/template)
add_subdirectory(amplp/examples/basic)
add_subdirectory(amplp/examples/amplcsv)
add_subdirectory(amplp/examples/eodbc)
add_subdirectory(amplp/examples/funclink)
add_subdirectory(amplp/examples/modhide)
add_subdirectory(amplp/examples/heuristic)
add_subdirectory(amplp/examples/ampljson)

# amplxl
add_subdirectory(amplxl)

# simpbit
add_ampl_library(simpbit tables/simpbit.c)
target_link_libraries(simpbit PUBLIC asl)

# fullbit
add_ampl_library(fullbit tables/fullbit.c)
target_link_libraries(fullbit PUBLIC asl)

add_c_compiler_flags(simpbit -Wno-parentheses)
add_c_compiler_flags(fullbit -Wno-parentheses)

# ampltabl
add_ampl_library(ampltabl tables/tableproxy.c tables/tableproxyver.h)
target_link_libraries(ampltabl asl)

# SQLite3
# find_package(SQLite3)
# if (SQLite3_FOUND)
#   add_ampl_library(sqlite3th sqlite3/handler.c)
#   set_property(TARGET sqlite3th PROPERTY C_STANDARD 99)
#   target_include_directories(sqlite3th PRIVATE ${SQLite3_INCLUDE_DIRS})
#   target_link_libraries(sqlite3th PUBLIC asl ${SQLite3_LIBRARIES})
#   file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sqlite3)
#   file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/tests)
#   add_test(NAME sqlite3-test COMMAND ${PYTHON_PROGRAM}
#     ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/tests/test.py ${AMPL_PROGRAM} $<TARGET_FILE:sqlite3th>)
# endif (SQLite3_FOUND)
  
find_package(ODBC)
if (ODBC_FOUND)
  add_library(amplodbc STATIC tables/amplodbc.c)
  set_property(TARGET amplodbc PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(amplodbc PRIVATE ${ODBC_INCLUDE_DIR})
  target_compile_definitions(amplodbc PRIVATE funcadd_ASL=funcaddodbc_ASL)
  add_c_compiler_flags(amplodbc -Wno-deprecated-declarations)
  target_link_libraries(amplodbc asl ${ODBC_LIBRARIES})
  target_link_libraries(ampltabl amplodbc)
  target_compile_definitions(ampltabl PRIVATE OTHER_FUNCADD=funcaddodbc_ASL)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TABLEPROXY_SUFFIX 64)
else ()
  set(TABLEPROXY_SUFFIX 32)
endif ()

add_executable(tableproxy
  tables/tableproxy.c ${ASL1_SOURCE_DIR}/printf.c  tables/tableproxyver.h)
set_target_properties(tableproxy
  PROPERTIES OUTPUT_NAME tableproxy${TABLEPROXY_SUFFIX})
target_compile_definitions(tableproxy PRIVATE STAND_ALONE=1 QUOTIFY=1)
target_link_libraries(tableproxy asl)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # add_c_compiler_flags cannot detect that GCC doesn't support
  # -Wno-string-plus-int, so add this option for clang only.
  add_c_compiler_flags(tableproxy -Wno-string-plus-int)
endif ()

# Link with system socket libraries.
if (WIN32)
  target_link_libraries(ampltabl wsock32)
  target_link_libraries(tableproxy wsock32)
  target_compile_definitions(tableproxy PRIVATE NO_sockaddr_in6)
elseif (CMAKE_SYSTEM_NAME MATCHES "SunOS")
  target_link_libraries(tableproxy socket nsl)
endif ()

# Suppress warnings in ASL and tableproxy.
if (MSVC)
  foreach (target asl ampltabl amplodbc
                  fullbit simpbit tableproxy)
    if (TARGET ${target})
      target_compile_options(${target}
        PRIVATE /wd4013 /wd4018 /wd4101 /wd4244 /wd4273 /wd4267 /wd4996)
    endif ()
  endforeach ()
else ()
  add_c_compiler_flags(asl -Wno-unused-result -Wno-parentheses)
endif ()

