set(SRCDIR src)
include_directories(../../include)
set(SOURCES ../../include/ampltableconnector.hpp
            ${SRCDIR}/handler.hpp
            ${SRCDIR}/handler.cpp
) 

add_ampl_library(eodbc ${SOURCES})

if(MSVC)
  target_compile_definitions(eodbc PRIVATE WIN32_LEAN_AND_MEAN)
endif()

set_target_properties(eodbc PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(eodbc ${ODBC_LIBRARIES})
target_include_directories(eodbc PRIVATE ${ODBC_INCLUDE_DIR})

# Link with system socket libraries.
if (WIN32)
  target_link_libraries(eodbc wsock32)
  target_compile_definitions(eodbc PRIVATE NO_sockaddr_in6)
elseif (CMAKE_SYSTEM_NAME MATCHES "SunOS")
  target_link_libraries(eodbc socket nsl)
endif ()
