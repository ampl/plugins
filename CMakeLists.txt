cmake_minimum_required(VERSION 3.4.0)

if(${CMAKE_VERSION} VERSION_LESS 3.11)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.11)
endif()

project(TABLES VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# CPack
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_VERSION ${ALIC_VERSION})
set(CPACK_PACKAGE_FILE_NAME tables)
include(CPack)

# CTest
include(CTest)
enable_testing()

set(AMPL_LIBRARY_DIR .)

# Adds a prefix to arguments.
function (add_prefix var prefix)
  set(result ${${var}})
  foreach (arg ${ARGN})
    set(result ${result} "${prefix}${arg}")
  endforeach ()
  set(${var} ${result} PARENT_SCOPE)
endfunction ()

function (add_mp_library name)
  cmake_parse_arguments(add_mp_library "OBJECT;STATIC" ""
    "COMPILE_DEFINITIONS;INCLUDE_DIRECTORIES;LIBRARIES;OBJECT_LIBRARIES;DEPENDS"
    ${ARGN})
  # Get object files.
  set(objects)
  set(dynrt-objects)
  foreach (lib ${add_mp_library_OBJECT_LIBRARIES})
    set(objects ${objects} $<TARGET_OBJECTS:${lib}>)
    set(dynrt-objects ${dynrt-objects} $<TARGET_OBJECTS:${lib}-dynrt>)
  endforeach ()
  # Add library.
  set(libtype )
  if (add_mp_library_OBJECT)
    set(libtype OBJECT)
  elseif (add_mp_library_STATIC)
    set(libtype STATIC)
  endif ()
  add_library(${name} ${libtype}
    ${add_mp_library_UNPARSED_ARGUMENTS} ${objects})
  target_compile_definitions(${name}
    PUBLIC ${add_mp_library_COMPILE_DEFINITIONS})
  target_include_directories(${name}
    PUBLIC ${add_mp_library_INCLUDE_DIRECTORIES})
  if (add_mp_library_DEPENDS)
    add_dependencies(${name} ${add_mp_library_DEPENDS})
  endif ()
  # Add library linked with dynamic runtime.
  if (MSVC)
    add_library(${name}-dynrt ${libtype} EXCLUDE_FROM_ALL
      ${add_mp_library_UNPARSED_ARGUMENTS} ${dynrt-objects})
    target_compile_options(${name}-dynrt PUBLIC /MD$<$<CONFIG:Debug>:d>)
    target_compile_definitions(${name}-dynrt
      PUBLIC ${add_mp_library_COMPILE_DEFINITIONS})
    target_include_directories(${name}-dynrt
      PUBLIC ${add_mp_library_INCLUDE_DIRECTORIES})
    if (add_mp_library_DEPENDS)
      add_dependencies(${name}-dynrt ${add_mp_library_DEPENDS})
    endif ()
  endif ()
  # Link libraries.
  foreach (lib ${add_mp_library_LIBRARIES})
    target_link_libraries(${name} ${lib})
    if (MSVC)
      target_link_libraries(${name}-dynrt ${lib}-dynrt)
    endif ()
  endforeach ()
endfunction ()

# Checks if the C compiler supports flag and if it does, adds it to the target.
include(CheckCCompilerFlag)
function (add_c_compiler_flags target)
  foreach (flag ${ARGN})
    string(REPLACE "-" "_" var ${flag})
    check_c_compiler_flag(${flag} ${var})
    if (${var})
      target_compile_options(${target} PRIVATE ${flag})
    endif ()
  endforeach ()
endfunction ()

# Adds a shared AMPL library which by convention doesn't have any prefix
# and has a suffix ".dll" on all platforms.
macro(add_ampl_library name)
  cmake_parse_arguments(add_ampl_library PRIVATE "" "" ${ARGN})
  add_library(${name} SHARED ${add_ampl_library_UNPARSED_ARGUMENTS})
  set_target_properties(${name} PROPERTIES PREFIX "")
  set_target_properties(${name} PROPERTIES SUFFIX ".dll")
  if (NOT add_ampl_library_PRIVATE)
    # Specify RUNTIME DESTINATION and LIBRARY DESTINATION, but not
    # DESTINATION or ARCHIVE_DESTINATION because we don't want import
    # libraries installed.
    install(TARGETS ${name}
        RUNTIME DESTINATION ${AMPL_LIBRARY_DIR}
        LIBRARY DESTINATION ${AMPL_LIBRARY_DIR}
        ARCHIVE DESTINATION ${AMPL_LIBRARY_DIR}
    )
  endif ()
endmacro()



file(READ solvers2/details.c0 DETAILS)
string(REPLACE "System_details"
  "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" DETAILS "${DETAILS}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/details.c "${DETAILS}")

configure_file(solvers2/stdio1.h0 stdio1.h COPYONLY)

# Generate arith.h
if (CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR MATCHES "^x86")
  include(CheckTypeSize)
  check_type_size(double DOUBLE_SIZE)
  check_type_size(long LONG_SIZE)
  set(ARITH_H "#define IEEE_8087\n#define Arith_Kind_ASL 1\n")
  set(ARITH_INT ) # Integer type used by arith.h checks.
  math(EXPR LONG_SIZE_X2 "${LONG_SIZE} * 2")
  if (DOUBLE_SIZE EQUAL LONG_SIZE_X2)
    set(ARITH_INT long)
    set(ARITH_INT_SIZE ${LONG_SIZE})
  else ()
    check_type_size(int INT_SIZE)
    math(EXPR INT_SIZE_X2 "${INT_SIZE} * 2")
    if (DOUBLE_SIZE EQUAL INT_SIZE_X2)
      set(ARITH_H "${ARITH_H}#define Long int\n#define Intcast (int)(long)\n")
      set(ARITH_INT int)
      set(ARITH_INT_SIZE ${INT_SIZE})
    endif ()
  endif ()
  if (ARITH_INT)
    check_type_size("struct { double d\; ${ARITH_INT} L\; }[2]" STRUCT_SIZE)
    math(EXPR DOUBLE_PLUS_INT_SIZE_X2
         "2 * (${DOUBLE_SIZE} + ${ARITH_INT_SIZE})")
    if (STRUCT_SIZE GREATER DOUBLE_PLUS_INT_SIZE_X2)
      set(ARITH_H "${ARITH_H}#define Double_Align\n")
    endif ()
  endif ()
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARITH_H "${ARITH_H}#define X64_bit_pointers\n")
  endif ()
  # Check long long.
  check_type_size("long long" LONG_LONG_SIZE)
  if (LONG_LONG_SIZE GREATER LONG_SIZE AND
      LONG_LONG_SIZE EQUAL CMAKE_SIZEOF_VOID_P)
    set(ARITH_H "${ARITH_H}#define LONG_LONG_POINTERS\n")
  endif ()
  if (LONG_LONG_SIZE LESS 8)
    set(ARITH_H "${ARITH_H}#define NO_LONG_LONG\n")
  endif ()
  # Check ssize_t.
  check_type_size(size_t SIZE_T_SIZE)
  check_type_size(ssize_t SSIZE_T_SIZE)
  if (NOT SSIZE_T_SIZE)
    if (SIZE_T_SIZE EQUAL LONG_SIZE)
      set(ARITH_SSIZE_T long)
    elseif (SIZE_T_SIZE EQUAL INT_SIZE)
      set(ARITH_SSIZE_T int)
    elseif (SIZE_T_SIZE EQUAL LONG_LONG_SIZE)
      set(ARITH_SSIZE_T "long long")
    else ()
      set(ARITH_SSIZE_T "signed size_t")
    endif ()
    set(ARITH_H "${ARITH_H}#define ssize_t ${ARITH_SSIZE_T}\n")
  elseif (NOT SIZE_T_SIZE EQUAL SSIZE_T_SIZE)
    set(ARITH_H "${ARITH_H}/* sizeof(size_t) = ${SIZE_T_SIZE}")
    set(ARITH_H "${ARITH_H} but sizeof(ssize_t) = ${SSIZE_T_SIZE} */\n")
  endif ()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/arith.h
       "${ARITH_H}#define QNaN0 0x0\n#define QNaN1 0xfff80000\n")
else ()
  if (NOT WIN32)
    set(FPINIT solvers2/fpinit.c)
  endif ()

  if (GENERATE_ARITH)
    add_executable(arithchk solvers2/arithchk.c ${FPINIT})
    if (WIN32)
      target_compile_definitions(arithchk PRIVATE NO_FPINIT NO_SSIZE_T)
    else ()
      target_compile_definitions(arithchk PRIVATE ASL_NO_FPINITMT)
      target_link_libraries(arithchk m)
    endif ()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arith.h DEPENDS arithchk
      COMMAND ${WINE} $<TARGET_FILE:arithchk> > arith.h COMMENT "Writing arith.h")
    if (NOT MSVC)
      string(REPLACE "-Wall -Wextra -pedantic" ""
        CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
      target_compile_options(arithchk PUBLIC -Wno-format-security)
    endif ()
  else ()
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arith.h
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/solvers2/arith.h1 ${CMAKE_CURRENT_BINARY_DIR}/arith.h
      COMMENT "Copying arith.h1 to arith.h")
  endif (GENERATE_ARITH)
endif ()

# Use a custom target for arith.h, because including a file generated by a
# custom command in more than one target may result in it being generated
# multiple times in a parallel build which doesn't work reliably in msbuild
# (#62). See also http://www.cmake.org/pipermail/cmake/2008-October/024492.html
add_custom_target(arith-h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/arith.h)

set(ASL_CORE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/details.c)
add_prefix(ASL_CORE_SOURCES solvers2/
    arithchk.c
    asldate.c
    asl.h
    asl_pfg.h
    asl_pfgh.h
    atof.c
    auxinfo.c
    avltree.c
    avltree.h
    basename.c
    bscanf.c
    b_search.c
    conscale.c
    degree.c
    derprop.c
    dtoa1.c
    # dtoa.c
    duthes.c
    dynlink.c
    errchk.h
    eval1.c
    eval2.c
    ewalloc1.c
    ewalloc2.c
    fg_read.c
    fg_write.c
    fpecatch.c
    fpinit.c
    # fpinitmt.c
    f_read.c
    fullhes.c
    funcadd0.c
    funcadd1.c
    func_add.c
    funcadd.c
    funcadd.h
    # funcaddk.c
    # funcaddr.c
    genrowno.c
    getenv.c
    getstub.c
    getstub.h
    g_fmt.c
    htcl.c
    indic_cons.c
    jac0dim.c
    jac2dim.c
    jac2dim.h
    jacdim.c
    jacinc1.c
    jacinc.c
    jacpdim.h
    libnamsave.c
    mach.c
    mainexit.c
    mip_pri.c
    misc.c
    mpec_adj0.c
    mpec_adj.c
    mqpcheckv.c
    mypow.c
    names.c
    nl_obj.c
    nlp2.h
    nlp.h
    nqpcheck.c
    nqpcheckZ.c
    obj_adj0.c
    obj_adj.c
    obj_adj.h
    objconst.c
    obj_prec.c
    objval_.c
    opno2.h
    op_type.c
    pfghread.c
    printf.c
    pshvprod.c
    psinfo.h
    punknown.c
    qpcheck.c
    qpcheckZ.c
    qsortv.c
    readsol.c
    repwhere.c
    sigcatch.c
    sjac0dim.c
    sos_add.c
    sphes.c
    sprintf.c
    sscanf.c
    stderr.c
    strerror.c
    studchk0.c
    suf_sos.c
    value.c
    writesol.c
    wrtsol_.c
    ws_desc.c
    wsu_desc.c
    xectim.c
    xp2known.c)

include(CheckSymbolExists)
include(CheckLibraryExists)

check_symbol_exists(mkstemps stdlib.h HAVE_MKSTEMPS)
if (NOT HAVE_MKSTEMPS)
  set(ASL_CORE_SOURCES ${ASL_CORE_SOURCES} mkstemps.c)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  # Changing the floating point precision is not supported on x64.
  set(ASL_COMPILE_DEFINITIONS No_Control87)
endif ()

check_symbol_exists(strtoull stdlib.h HAVE_STRTOULL)
if (NOT HAVE_STRTOULL)
  check_symbol_exists(_strtoui64 stdlib.h HAVE_STRTOUI64)
  if (HAVE_STRTOUI64)
    set(ASL_COMPILE_DEFINITIONS ${ASL_COMPILE_DEFINITIONS} strtoull=_strtoui64)
    set(MP_MEX_OPTIONS "-Dstrtoull=_strtoui64")
  endif ()
endif ()

# Compile C sources separately to be able to specify compile options.
add_mp_library(asl-core OBJECT ${ASL_CORE_SOURCES} DEPENDS arith-h
  COMPILE_DEFINITIONS ${ASL_COMPILE_DEFINITIONS}
  INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

set(ASL_INCLUDE_DIRS solvers2 ${CMAKE_CURRENT_BINARY_DIR})
add_mp_library(asl STATIC OBJECT_LIBRARIES asl-core
  INCLUDE_DIRECTORIES ${ASL_INCLUDE_DIRS})

if (NOT WIN32)
  target_link_libraries(asl m)
endif ()

if (BUILD_SHARED_LIBS)
  add_library(asl-shared $<TARGET_OBJECTS:asl-core>)
  set_target_properties(asl-shared PROPERTIES
    VERSION ${MP_VERSION} SOVERSION ${MP_VERSION_MAJOR})
  set_target_properties(asl-shared PROPERTIES OUTPUT_NAME asl)
  target_link_libraries(asl-shared asl)
  install(TARGETS asl-shared DESTINATION lib RUNTIME DESTINATION bin)
endif ()

check_symbol_exists(mkstemps stdlib.h HAVE_MKSTEMPS)
if (HAVE_MKSTEMPS)
  target_compile_definitions(asl PRIVATE MP_HAVE_MKSTEMPS=1)
endif ()

target_link_libraries(asl ${CMAKE_DL_LIBS})

# simpbit
add_ampl_library(simpbit tables/simpbit.c)
target_link_libraries(simpbit PUBLIC asl)

# fullbit
add_ampl_library(fullbit tables/fullbit.c)
target_link_libraries(fullbit PUBLIC asl)

add_c_compiler_flags(simpbit -Wno-parentheses)
add_c_compiler_flags(fullbit -Wno-parentheses)

# ampltabl
add_ampl_library(ampltabl tables/tableproxy.c tables/tableproxyver.h)

# SQLite3
find_package(SQLite3)
if (SQLite3_FOUND)
  add_ampl_library(sqlite3 sqlite3/handler.c)
  target_include_directories(sqlite3 PRIVATE ${SQLite3_INCLUDE_DIRS})
  target_link_libraries(sqlite3 PUBLIC asl ${SQLite3_LIBRARIES})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sqlite3)
  add_test(NAME sqlite3-test-1 COMMAND ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/tests/test-diet-1.sh WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
  add_test(NAME sqlite3-test-2 COMMAND ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/tests/test-diet-2.sh WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
endif (SQLite3_FOUND)

find_package(ODBC)
if (ODBC_FOUND)
  add_library(amplodbc STATIC tables/amplodbc.c)
  set_property(TARGET amplodbc PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(amplodbc PRIVATE ${ODBC_INCLUDE_DIR})
  target_compile_definitions(amplodbc PRIVATE funcadd_ASL=funcaddodbc_ASL)
  add_c_compiler_flags(amplodbc -Wno-deprecated-declarations)
  target_link_libraries(amplodbc asl ${ODBC_LIBRARIES})
  target_link_libraries(ampltabl asl amplodbc)
  target_compile_definitions(ampltabl PRIVATE OTHER_FUNCADD=funcaddodbc_ASL)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TABLEPROXY_SUFFIX 64)
else ()
  set(TABLEPROXY_SUFFIX 32)
endif ()

add_executable(tableproxy
  tables/tableproxy.c solvers2/printf.c tables/tableproxyver.h)
set_target_properties(tableproxy
  PROPERTIES OUTPUT_NAME tableproxy${TABLEPROXY_SUFFIX})
target_compile_definitions(tableproxy PRIVATE STAND_ALONE=1 QUOTIFY=1)
target_link_libraries(tableproxy asl)
if (MP_CLANG)
  # add_c_compiler_flags cannot detect that GCC doesn't support
  # -Wno-string-plus-int, so add this option for clang only.
  add_c_compiler_flags(tableproxy -Wno-string-plus-int)
endif ()

# Link with system socket libraries.
if (WIN32)
  target_link_libraries(ampltabl wsock32)
  target_link_libraries(tableproxy wsock32)
  target_compile_definitions(tableproxy PRIVATE NO_sockaddr_in6)
elseif (CMAKE_SYSTEM_NAME MATCHES "SunOS")
  target_link_libraries(tableproxy socket nsl)
endif ()

# Suppress warnings in ASL and tableproxy.
if (MSVC)
  foreach (target asl-core asl-core-dynrt gjh ampltabl amplodbc
                  fullbit simpbit tableproxy)
    if (TARGET ${target})
      target_compile_options(${target}
        PRIVATE /wd4013 /wd4018 /wd4101 /wd4244 /wd4273 /wd4267 /wd4996)
    endif ()
  endforeach ()
else ()
  add_c_compiler_flags(asl-core -Wno-unused-result -Wno-parentheses)
endif ()

    
