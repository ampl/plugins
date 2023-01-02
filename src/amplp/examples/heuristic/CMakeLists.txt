set(AMPLP_DIR ../../include)

set(SRCDIR src)
set(SOURCES ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/funcadd.cpp
			${SRCDIR}/knap.cpp
)

add_ampl_library(heuristic ${SOURCES})

set_target_properties(heuristic PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET heuristic PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(heuristic PRIVATE ${AMPLP_DIR})

if(MSVC)
  target_compile_definitions(heuristic PRIVATE WIN32_LEAN_AND_MEAN)
endif()
