set(AMPLT_DIR ../../include)

set(SRCDIR src)
set(SOURCES ${AMPLT_DIR}/ampltableconnector.hpp
			${SRCDIR}/funcadd.cpp
)

add_ampl_library(funclink ${SOURCES})

set_target_properties(funclink PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET funclink PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(funclink PRIVATE ${AMPLT_DIR})

if(MSVC)
  target_compile_definitions(funclink PRIVATE WIN32_LEAN_AND_MEAN)
endif()
