set(AMPLP_DIR ../../include)

set(SRCDIR src)
set(SOURCES_ENC ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/trivial_encoder.cpp
)

set(SOURCES_DEC ${AMPLP_DIR}/amplp.hpp
			${SRCDIR}/trivial_decoder.cpp
)

add_executable(trivial_encoder ${SOURCES_ENC})
add_ampl_library(trivial_decoder ${SOURCES_DEC})

set_target_properties(trivial_encoder PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(trivial_decoder PROPERTIES LINKER_LANGUAGE CXX)

set_property(TARGET trivial_encoder PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET trivial_decoder PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(trivial_encoder PRIVATE ${AMPLP_DIR})
target_include_directories(trivial_decoder PRIVATE ${AMPLP_DIR})

if(MSVC)
  target_compile_definitions(trivial_encoder PRIVATE WIN32_LEAN_AND_MEAN)
  target_compile_definitions(trivial_decoder PRIVATE WIN32_LEAN_AND_MEAN)
endif()
