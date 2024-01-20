# Build zlib from the included sources, it generates the target
# zlibstatic that we can link statically to
set(SKIP_INSTALL_ALL ON) # skip install targets of zlib

set(Z_DIR zlib-1.2.11)
set(MZ_DIR ${Z_DIR}/contrib/minizip)

add_subdirectory(${Z_DIR})
addToTablesFolder(support zlib zlibstatic minigzip)
set(ZLIB_INCLUDE_DIRS ./${Z_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${Z_DIR} ./${MZ_DIR})
set(ZLIB_LIBRARIES zlibstatic)

set(AMPLXL_SOURCES 
    ./src/ampl_xl.cpp 
    ./src/myunz.cpp
    ./src/myzip.cpp
    ./src/copyzip.cpp
    ./src/pugixml.cpp
    ./src/oxmlutils.cpp
    ./src/logger.cpp
    ./src/utils.cpp
    ./${MZ_DIR}/ioapi.c
    ./${MZ_DIR}/zip.c
    ./${MZ_DIR}/unzip.c)

if (WIN32)
  set(AMPLXL_SOURCES ${AMPLXL_SOURCES} ./${MZ_DIR}//iowin32.c)
endif()

add_ampl_library(amplxl ${AMPLXL_SOURCES}
                COMPONENT ${DISTRO_COMPONENT})

set_target_properties(amplxl PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(amplxl PRIVATE asl2 ${ZLIB_LIBRARIES})
target_include_directories(amplxl PRIVATE ${ZLIB_INCLUDE_DIRS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/amplxl)
file(COPY ../test_utils.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/amplxl/tests)
add_test(NAME amplxl-test COMMAND ${PYTHON_PROGRAM}
  ${CMAKE_CURRENT_BINARY_DIR}/amplxl/tests/test.py ${AMPL_PROGRAM} $<TARGET_FILE:amplxl>)
