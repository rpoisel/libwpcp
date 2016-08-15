cmake_minimum_required(VERSION 2.8)

project(libwpcp C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_GENERATED_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


if (MSVC)
  set(ADDITIONAL_COMPILER_FLAGS "/Wall /wd4820")

  if (MSVC_VERSION LESS 1600)
    set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} /wd4255")
    include_directories(win32compat)
  endif ()

  if (NOT MSVC_VERSION LESS 1500)
    set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} /MP")
  endif ()
else ()
  set(ADDITIONAL_COMPILER_FLAGS "-fvisibility=hidden -std=c99 -W -Wall -Wextra -fPIC")

  if (WPCP_GENERATE_COVERAGE)
    set(ADDITIONAL_FLAGS_DEBUG "-fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${ADDITIONAL_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${ADDITIONAL_FLAGS_DEBUG}")
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -lgcov")
    else ()
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
    endif ()
  endif()
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ADDITIONAL_COMPILER_FLAGS}")


option(WPCP_BUILD_CHECK "Build Check as ExternalProject" OFF)
option(WPCP_BUILD_LIBWEBSOCKETS "Build LibWebSockets as ExternalProject" OFF)


include(ExternalProject)

if (WPCP_BUILD_LIBWEBSOCKETS)
  ExternalProject_Add(
    libwebsockets
    GIT_REPOSITORY git://git.libwebsockets.org/libwebsockets
    GIT_TAG "v1.6.0-chrome48-firefox42"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}" "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}" "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>" "-DLWS_WITHOUT_EXTENSIONS=ON" "-DLWS_WITHOUT_TESTAPPS=ON" "-DLWS_WITH_SSL:BOOL=ON"
  )

  set(LIBWEBSOCKETS_TARGET_NAME websockets${CMAKE_STATIC_LIBRARY_SUFFIX})

  add_library(websockets STATIC IMPORTED)
  add_dependencies(websockets libwebsockets)
  ExternalProject_Get_Property(libwebsockets INSTALL_DIR)
  set_property(TARGET websockets PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBWEBSOCKETS_TARGET_NAME})

  set(LIBWEBSOCKETS_INCLUDE_DIRS ${INSTALL_DIR}/include)
  set(LIBWEBSOCKETS_LIBRARIES websockets)

  find_package(OpenSSL REQUIRED)
  list(APPEND LIBWEBSOCKETS_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
  list(APPEND LIBWEBSOCKETS_LIBRARIES ${OPENSSL_LIBRARIES})
else ()
  find_package(LibWebSockets)
  if (NOT LIBWEBSOCKETS_FOUND)
    message(FATAL_ERROR "Call cmake with '-DWPCP_BUILD_LIBWEBSOCKETS=ON' to build LibWebSockets as ExternalProject.")
  endif ()
endif ()

if (WPCP_BUILD_CHECK)
  ExternalProject_Add(
    check-code
    URL http://sourceforge.net/projects/check/files/check/0.10.0/check-0.10.0.tar.gz/download
    URL_HASH SHA256=f5f50766aa6f8fe5a2df752666ca01a950add45079aa06416b83765b1cf71052
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:PATH=${CMAKE_BUILD_TYPE}" "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}" "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
    BUILD_IN_SOURCE 1
  )

  add_library(check STATIC IMPORTED)
  add_library(compat STATIC IMPORTED)
  add_dependencies(check check-code)
  ExternalProject_Get_Property(check-code INSTALL_DIR)
  ExternalProject_Get_Property(check-code SOURCE_DIR)
  set_property(TARGET check PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}check${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_property(TARGET compat PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}compat${CMAKE_STATIC_LIBRARY_SUFFIX})

  set(CHECK_INCLUDE_DIRS ${INSTALL_DIR}/include ${SOURCE_DIR})
  set(CHECK_LIBRARIES check compat)
else ()
  find_package(Check)
  find_package(Subunit)
  if (NOT CHECK_FOUND)
    message(FATAL_ERROR "Call cmake with '-DWPCP_BUILD_CHECK=ON' to build Check as ExternalProject.")
  endif ()
  if (SUBUNIT_FOUND)
    list(APPEND CHECK_LIBRARIES ${SUBUNIT_LIBRARIES})
  endif ()
endif ()

if (UNIX AND NOT APPLE)
  list(APPEND CHECK_LIBRARIES m pthread rt)
endif ()


if (WPCP_GENERATE_COVERAGE)
  add_definitions(-DWPCP_DISABLE_ASSERT)
endif ()


include_directories(${CHECK_INCLUDE_DIRS} ${LIBWEBSOCKETS_INCLUDE_DIRS} include src)

set(WPCP_PUBLIC_HEADERS
  include/wpcp.h
)

set(WPCP_PRIVATE_HEADERS
  src/wpcp_cbor.h
  src/wpcp_session.h
  src/wpcp_util.h
)

set(WPCP_SOURCES
  src/wpcp.c
  src/wpcp_cbor.c
  src/wpcp_session.c
)

set(WPCP_LWS_PUBLIC_HEADERS
  include/wpcp_lws.h
)

set(WPCP_LWS_SOURCES
  src/wpcp_lws.c
)

set(WPCP_TEST_FILES
  test/test.h
  test/test.c
  test/test_cbor_read_buffer.c
  test/test_cbor_write.c
  test/test_cbor_write_buffer.c
  test/test_session.c
  test/test_wpcp.c
)

set(EXAMPLE_SOURCES
  example/example.c
)

set(WPCP_LIBRARIES "")

if (WINCE)
  list(APPEND WPCP_LIBRARIES ws2)
elseif (WIN32)
  list(APPEND WPCP_LIBRARIES ws2_32)
endif ()

source_group("Headers Private" FILES ${WPCP_PRIVATE_HEADERS})
source_group("Headers Public" FILES ${WPCP_PUBLIC_HEADERS} ${WPCP_LWS_PUBLIC_HEADERS})
source_group("Sources" FILES ${WPCP_SOURCES} ${WPCP_LWS_SOURCES})
source_group("Tests" FILES ${WPCP_TEST_FILES})

add_library(wpcp SHARED ${WPCP_PUBLIC_HEADERS} ${WPCP_PRIVATE_HEADERS} ${WPCP_SOURCES})
set_property(TARGET wpcp PROPERTY COMPILE_DEFINITIONS WPCP_EXPORTS)
set_property(TARGET wpcp PROPERTY PUBLIC_HEADER ${WPCP_PUBLIC_HEADERS})
target_link_libraries(wpcp ${WPCP_LIBRARIES})
install(TARGETS wpcp DESTINATION lib)

add_library(wpcp-lws SHARED ${WPCP_LWS_PUBLIC_HEADERS} ${WPCP_LWS_SOURCES})
set_property(TARGET wpcp-lws PROPERTY COMPILE_DEFINITIONS WPCP_LWS_EXPORTS)
set_property(TARGET wpcp-lws PROPERTY PUBLIC_HEADER ${WPCP_LWS_PUBLIC_HEADERS})
target_link_libraries(wpcp-lws wpcp ${LIBWEBSOCKETS_LIBRARIES})
if (NOT WIN32)
  target_link_libraries(wpcp-lws pthread)
endif ()
install(TARGETS wpcp-lws DESTINATION lib)

add_executable(example ${EXAMPLE_SOURCES})
target_link_libraries(example wpcp-lws)

add_executable(test_wpcp ${WPCP_TEST_FILES} ${WPCP_PUBLIC_HEADERS} ${WPCP_PRIVATE_HEADERS} ${WPCP_SOURCES})
set_property(TARGET test_wpcp PROPERTY COMPILE_DEFINITIONS WPCP_STATIC WPCP_LWS_STATIC WPCP_USE_CUSTOM_ALLOCATOR)
target_link_libraries(test_wpcp ${CHECK_LIBRARIES} ${WPCP_LIBRARIES})

enable_testing()
get_property(TEST_WPCP_LOCATION TARGET test_wpcp PROPERTY LOCATION)
add_test(wpcp ${TEST_WPCP_LOCATION})


find_program(GENHTML_EXECUTABLE NAMES genhtml)
find_program(LCOV_EXECUTABLE NAMES lcov)
if (GENHTML_EXECUTABLE AND LCOV_EXECUTABLE)
  add_custom_target(coverage_reset
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --zerocounters
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --capture --initial --output-file ${CMAKE_BINARY_DIR}/coverage_base.info
  )
  add_custom_target(coverage_generate
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage_test.info
    COMMAND lcov --extract ${CMAKE_BINARY_DIR}/coverage_test.info "${CMAKE_SOURCE_DIR}/src/*" --output-file ${CMAKE_BINARY_DIR}/coverage_test.info
    COMMAND ${GENHTML_EXECUTABLE} --baseline-file ${CMAKE_BINARY_DIR}/coverage_base.info --title ${CMAKE_PROJECT_NAME} --legend --show-details --num-spaces 4 --output-directory ${CMAKE_BINARY_DIR}/coverage ${CMAKE_BINARY_DIR}/coverage_test.info
  )
else ()
  message(STATUS "Can't generate coverage report: genhtml and/or lcov was not found")
endif ()

find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
find_program(CPPCHECKHTMLREPORT_EXECUTABLE NAMES cppcheck-htmlreport)
if (CPPCHECK_EXECUTABLE AND CPPCHECKHTMLREPORT_EXECUTABLE)
  add_custom_target(cppcheck
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE} --enable=style --force -j 8 --verbose --xml ${CMAKE_SOURCE_DIR} 3>&1 1>&2 2>&3 | ${CPPCHECKHTMLREPORT_EXECUTABLE} --title=${CMAKE_PROJECT_NAME} --report-dir=${CMAKE_BINARY_DIR}/cppcheck
  )
else ()
  message(STATUS "Can't run cppchek: cppcheck and/or cppcheck-htmlreport was not found")
endif ()
