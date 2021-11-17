set(AMPLP_DIR ../../include)

set(SRCDIR src)
set(SOURCES ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/handler.hpp
			${SRCDIR}/handler.cpp
) 

add_ampl_library(template ${SOURCES})
target_include_directories(template PRIVATE ${AMPLP_DIR})

if(MSVC)
    target_compile_definitions(template PRIVATE WIN32_LEAN_AND_MEAN)
endif()
