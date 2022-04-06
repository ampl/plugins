set(AMPLP_DIR ../../include)

set(SRCDIR src)
set(SOURCES ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/handler.hpp
			${SRCDIR}/handler.cpp
)

add_ampl_library(amplcsv ${SOURCES} COMPONENT ${DISTRO_COMPONENT})
target_include_directories(amplcsv PRIVATE ${AMPLP_DIR})

if(MSVC)
    target_compile_definitions(template PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# tests
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ../../../test_utils.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests)
add_test(NAME amplcsv-test COMMAND ${PYTHON_PROGRAM}
  ${CMAKE_CURRENT_BINARY_DIR}/tests/test.py ${AMPL_PROGRAM} $<TARGET_FILE:amplcsv>)
