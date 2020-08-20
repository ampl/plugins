set(SRCDIR src)
include_directories(../../include)
set(SOURCES ../../include/ampltableconnector.hpp
            ${SRCDIR}/handler.hpp
            ${SRCDIR}/handler.cpp
) 

add_ampl_library(template ${SOURCES})

if(MSVC)
    target_compile_definitions(template PRIVATE WIN32_LEAN_AND_MEAN)
endif()
