set(AMPLP_INC ../../include)
set(SQLITE3_INC ./include)

set(SOURCES src/handler.cpp
			sqlite/sqlite3.c
)

add_ampl_library(amplsqlite ${SOURCES} COMPONENT ${DISTRO_COMPONENT})

set_target_properties(amplsqlite PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET amplsqlite PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(amplsqlite PRIVATE ${AMPLP_INC})
target_include_directories(amplsqlite PRIVATE ${SQLITE3_INC})

if(MSVC)
  target_compile_definitions(amplsqlite PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# Link with system socket libraries.
if(WIN32)
	target_link_libraries(amplsqlite wsock32)
	target_compile_definitions(amplsqlite PRIVATE NO_sockaddr_in6)
elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
	target_link_libraries(amplsqlite socket nsl)
endif()
