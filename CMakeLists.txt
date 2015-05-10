cmake_minimum_required(VERSION 2.8)
project(IMAPcopy CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "You need at least version 4.8 when compiling with GCC")
  endif()
endif()

include (${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

if(  (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL   "4.9"
                                             OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.9"))
   OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
     set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/ragel")

find_package(RAGEL 6.6 REQUIRED)

# for out of source tree builds - when ragel generated files
# are not in the same dir as the source (include "foo.h"->foo.h can't be found)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost 1.55
  COMPONENTS
    system # needed by filesystem, log
    filesystem
    regex
    unit_test_framework
    program_options
    serialization
    log
    log_setup
    # needed by log
    thread
    #
    locale
    regex
  REQUIRED)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
  ${CMAKE_CURRENT_SOURCE_DIR}/libbuffer
  )

RAGEL_TARGET(imap_client_parser imap/client_parser.rl ${CMAKE_CURRENT_BINARY_DIR}/imap_client_parser.cc COMPILE_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})
RAGEL_TARGET(imap_server_parser imap/server_parser.rl ${CMAKE_CURRENT_BINARY_DIR}/imap_server_parser.cc COMPILE_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})

RAGEL_TARGET(mime_base64_decoder mime/base64_decoder.rl ${CMAKE_CURRENT_BINARY_DIR}/mime_base64_decoder.cc)
RAGEL_TARGET(mime_base64_decoder_main mime/base64_decoder_main.rl ${CMAKE_CURRENT_BINARY_DIR}/mime_base64_decoder_main.cc)
RAGEL_TARGET(mime_q_decoder mime/q_decoder.rl ${CMAKE_CURRENT_BINARY_DIR}/mime_q_decoder.cc)
RAGEL_TARGET(mime_q_decoder_main mime/q_decoder_main.rl ${CMAKE_CURRENT_BINARY_DIR}/mime_q_decoder_main.cc)
RAGEL_TARGET(mime_header_decoder mime/header_decoder.rl ${CMAKE_CURRENT_BINARY_DIR}/mime_header_decoder.cc)
RAGEL_TARGET(ascii_control_sanitizer ascii/control_sanitizer.rl ${CMAKE_CURRENT_BINARY_DIR}/ascii_control_sanitizer.cc)

add_subdirectory(libixxx)
add_subdirectory(libbuffer)

find_package(OpenSSL REQUIRED)
find_library(LIB_BOTAN
  NAMES botan
        botan-1.10 # for Fedora 20
  )
find_path(HEADER_BOTAN
  NAMES botan/pipe.h
  # e.g. Fedora 20 stores the botan header files
  # under /usr/include/botan-1.10/botan/*.h ...
  PATH_SUFFIXES botan-1.10
  )
include_directories(
  ${HEADER_BOTAN}
  )

add_executable(ut
  imap/imap.cc
  imap/client_parser_callback.cc
  imap/client_writer.cc
  imap/client_base.cc
  maildir/maildir.cc
  net/ssl_util.cc
  unittest/main.cc
  unittest/imap_client_parser.cc
  unittest/imap_server_parser.cc
  unittest/data.cc
  unittest/maildir.cc
  unittest/replay.cc
  unittest/imap_client_writer.cc
  example/server.cc
  example/client.cc
  ${RAGEL_imap_client_parser_OUTPUTS}
  ${RAGEL_imap_server_parser_OUTPUTS}
  lex_util.cc
  unittest/sequence_set.cc
  sequence_set.cc

  # for imapdl
  unittest/copy.cc
  copy/options.cc
  copy/client.cc
  copy/id.cc
  copy/journal.cc
  copy/state.cc
  copy/fetch_timer.cc
  copy/header_printer.cc
  net/client.cc
  net/client_application.cc
  net/tcp_client.cc
  trace/trace.cc
  log/log.cc
  net/ssl_verification.cc

  ${RAGEL_mime_base64_decoder_main_OUTPUTS}
  ${RAGEL_mime_q_decoder_main_OUTPUTS}
  ${RAGEL_mime_header_decoder_OUTPUTS}
  ${RAGEL_ascii_control_sanitizer_OUTPUTS}
  unittest/mime.cc
  unittest/lex_util.cc
  )
target_link_libraries(ut
  ${Boost_LIBRARIES}
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  buffer_static ixxx_static
  # for ut comparison
  ${LIB_BOTAN}

  )
SET_TARGET_PROPERTIES(ut
  PROPERTIES LINK_FLAGS "-pthread")

RAGEL_TARGET(length example/length.rl ${CMAKE_CURRENT_BINARY_DIR}/length.cc)

add_executable(length ${RAGEL_length_OUTPUTS})
target_link_libraries(length buffer_static ixxx_static)


add_executable(client
  example/client.cc
  example/client_main.cc
  net/ssl_util.cc
  )
target_link_libraries(client
  ixxx_static
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SERIALIZATION_LIBRARY}
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  )

add_executable(replay
  example/replay.cc
  trace/trace.cc
  )
target_link_libraries(replay
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_SERIALIZATION_LIBRARY}
  )

add_executable(server
  example/server.cc
  example/server_main.cc
  net/ssl_util.cc
  lex_util.cc
  trace/trace.cc
  )
target_link_libraries(server
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SERIALIZATION_LIBRARY}
  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  )

# otherwise link error with boost log
add_definitions(-DBOOST_LOG_DYN_LINK)

add_executable(imapdl
  copy/main.cc
  copy/options.cc
  copy/client.cc
  copy/id.cc
  copy/journal.cc
  copy/state.cc
  copy/fetch_timer.cc
  copy/header_printer.cc
  net/client.cc
  net/client_application.cc
  net/tcp_client.cc
  net/ssl_util.cc
  net/ssl_verification.cc
  log/log.cc
  imap/imap.cc
  ${RAGEL_imap_client_parser_OUTPUTS}
  lex_util.cc
  imap/client_parser_callback.cc
  imap/client_writer.cc
  imap/client_base.cc
  ${RAGEL_imap_server_parser_OUTPUTS}
  maildir/maildir.cc
  sequence_set.cc
  trace/trace.cc
  ${RAGEL_mime_header_decoder_OUTPUTS}
  ${RAGEL_ascii_control_sanitizer_OUTPUTS}
  )
target_link_libraries(imapdl
  ixxx_static
  buffer_static
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SERIALIZATION_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}

  ${Boost_LOG_LIBRARY}
  ${Boost_LOG_SETUP_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_LOCALE_LIBRARY}
  ${Boost_REGEX_LIBRARY}

  ${OPENSSL_SSL_LIBRARY}
  ${OPENSSL_CRYPTO_LIBRARY}
  )
SET_TARGET_PROPERTIES(imapdl
  PROPERTIES LINK_FLAGS "-pthread")


add_executable(hash
  example/hash.cc
  )
target_link_libraries(hash
  ${LIB_BOTAN}
  )
