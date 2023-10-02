set(AMPLP_DIR ../../include)
set(RAPID_JSON_DIR ./)

set(SRCDIR src)
set(SOURCES ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/handler.hpp
			${SRCDIR}/handler.cpp
)

add_ampl_library(ampljson ${SOURCES} COMPONENT ${DISTRO_COMPONENT})

set_target_properties(ampljson PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET ampljson PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(ampljson PRIVATE ${AMPLP_DIR})
target_include_directories(ampljson PRIVATE ${RAPID_JSON_DIR})

if(MSVC)
  target_compile_definitions(ampljson PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# Link with system socket libraries.
if(WIN32)
	target_link_libraries(ampljson wsock32)
	target_compile_definitions(ampljson PRIVATE NO_sockaddr_in6)
elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
	target_link_libraries(ampljson socket nsl)
endif()
