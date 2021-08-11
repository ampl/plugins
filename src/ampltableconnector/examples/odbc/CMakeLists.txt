find_package(ODBC)

if(ODBC_FOUND)

	message(STATUS "eodbc: ODBC found")
	message(STATUS "ODBC_INCLUDE_DIR: ${ODBC_INCLUDE_DIR}")
	message(STATUS "ODBC_LIBRARIES: ${ODBC_LIBRARIES}")

	set(AMPLT_DIR ../../include)

	set(SRCDIR src)
	set(SOURCES ${AMPLT_DIR}/ampltableconnector.hpp
				${SRCDIR}/handler.hpp
				${SRCDIR}/handler.cpp
	)

	add_ampl_library(eodbc ${SOURCES})

	set_target_properties(eodbc PROPERTIES LINKER_LANGUAGE CXX)
	set_property(TARGET eodbc PROPERTY POSITION_INDEPENDENT_CODE ON)

	target_include_directories(eodbc PRIVATE ${AMPLT_DIR})
	target_include_directories(eodbc PRIVATE ${ODBC_INCLUDE_DIR})

	target_link_libraries(eodbc ${ODBC_LIBRARIES})

	if(MSVC)
	  target_compile_definitions(eodbc PRIVATE WIN32_LEAN_AND_MEAN)
	endif()

	# Link with system socket libraries.
	if(WIN32)
		target_link_libraries(eodbc wsock32)
		target_compile_definitions(eodbc PRIVATE NO_sockaddr_in6)
	elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
		target_link_libraries(eodbc socket nsl)
	endif()

else()
	message(STATUS "eodbc: Cannot find ODBC...")
endif()
