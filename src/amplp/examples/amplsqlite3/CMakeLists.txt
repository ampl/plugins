set(AMPLP_INC ../../include)
set(SQLITE3_INC ./include)

set(SOURCES src/handler.cpp
			sqlite/sqlite3.c
)

add_ampl_library(amplsqlite3 ${SOURCES} COMPONENT ${DISTRO_COMPONENT})

set_target_properties(amplsqlite3 PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET amplsqlite3 PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(amplsqlite3 PRIVATE ${AMPLP_INC})
target_include_directories(amplsqlite3 PRIVATE ${SQLITE3_INC})

if(MSVC)
  target_compile_definitions(amplsqlite3 PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# tests
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ../../../test_utils.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests)
file(COPY ../../../test.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests)
add_test(NAME amplsqlite3-test COMMAND ${PYTHON_PROGRAM}
  ${CMAKE_CURRENT_BINARY_DIR}/tests/test.py ${AMPL_PROGRAM} $<TARGET_FILE:amplsqlite3>)
