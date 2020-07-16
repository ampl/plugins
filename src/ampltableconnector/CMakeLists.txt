set(SRCDIR src)
set(SOURCES ${SRCDIR}/ampltableconnector.cpp 
            ${SRCDIR}/logger.cpp 
            ${SRCDIR}/utils.cpp
            ${SRCDIR}/ampltableconnector.hpp 
            ${SRCDIR}/logger.hpp 
            ${SRCDIR}/utils.hpp
) 

add_ampl_library(ampltableconnector ${SOURCES})
target_include_directories(ampltableconnector PRIVATE ${ASL2_INCLUDE_DIRS} ${SRCDIR})

if(MSVC)
    target_compile_definitions(ampltableconnector PRIVATE WIN32_LEAN_AND_MEAN)
endif()