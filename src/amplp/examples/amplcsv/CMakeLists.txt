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
