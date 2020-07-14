# Copyright (c) 2016-2020 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.8)

# kth-blockchain
#==============================================================================
project(kth-blockchain
  VERSION 0
  LANGUAGES CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check for baseline language coverage in the compiler for the C++14 standard.
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Process options.
#==============================================================================

# Implement --use-conan
#------------------------------------------------------------------------------
option(USE_CONAN "Use Conan Build Tool." OFF)
option(NO_CONAN_AT_ALL "Conan totally disabled." OFF)

if (NO_CONAN_AT_ALL)
  set(USE_CONAN OFF)
endif()

# Inherit --enable-shared and define BOOST_TEST_DYN_LINK.
#------------------------------------------------------------------------------
option(ENABLE_SHARED "" OFF)

# Implement POSITION_INDEPENDENT_CODE
#------------------------------------------------------------------------------
option(ENABLE_POSITION_INDEPENDENT_CODE "Enable POSITION_INDEPENDENT_CODE property" ON)

# Implement --with-consensus and define WITH_CONSENSUS.
#------------------------------------------------------------------------------
option(WITH_CONSENSUS "Link kth-consensus and use for consensus checks." ON)

# Implement --with-tests and declare WITH_TESTS.
#------------------------------------------------------------------------------
option(WITH_TESTS "Compile with unit tests." ON)

# Implement --with-tools and declare WITH_TOOLS.
#------------------------------------------------------------------------------
option(WITH_TOOLS "Compile with tools." OFF)

# Implement --with-keoken and declare WITH_KEOKEN.
#------------------------------------------------------------------------------
option(WITH_KEOKEN "Keoken enabled." OFF)
if (WITH_KEOKEN)
  message(STATUS "Knuth: Keoken enabled")
  add_definitions(-DKTH_WITH_KEOKEN)
endif()

# # Implement --with-mining and declare WITH_MINING.
# #------------------------------------------------------------------------------
# option(WITH_MINING "Mining enabled." OFF)
# if (WITH_MINING)
#   message(STATUS "Knuth: Mining enabled")
#   add_definitions(-DKTH_WITH_MINING)
# endif()

# Implement --with-mempool and declare WITH_MEMPOOL.
#------------------------------------------------------------------------------
option(WITH_MEMPOOL "Mempool enabled." OFF)
if (WITH_MEMPOOL)
  message(STATUS "Knuth: Mempool enabled")
  add_definitions(-DKTH_WITH_MEMPOOL)
endif()

# Implement and declare DB_TRANSACTION_UNCONFIRMED.
#------------------------------------------------------------------------------
option(DB_TRANSACTION_UNCONFIRMED "DB Transactions Unconfirmed enabled." ON)
if (DB_TRANSACTION_UNCONFIRMED)
  message(STATUS "Knuth: DB Transactions Unconfirmed enabled")
  add_definitions(-DKTH_DB_TRANSACTION_UNCONFIRMED)
endif()

# Implement and declare DB_SPENDS.
#------------------------------------------------------------------------------
option(DB_SPENDS "DB Spends enabled." ON)
if (DB_SPENDS)
  message(STATUS "Knuth: DB Spends enabled")
  add_definitions(-DKTH_DB_SPENDS)
endif()

# Implement and declare DB_HISTORY.
#------------------------------------------------------------------------------
option(DB_HISTORY "DB History enabled." ON)
if (DB_HISTORY)
  message(STATUS "Knuth: DB History enabled")
  add_definitions(-DKTH_DB_HISTORY)
endif()

# Implement and declare DB_STEALTH.
#------------------------------------------------------------------------------
option(DB_STEALTH "DB Stealth enabled." ON)
if (DB_STEALTH)
  message(STATUS "Knuth: DB Stealth enabled")
  add_definitions(-DKTH_DB_STEALTH)
endif()

# Implement and declare DB_UNSPENT_LEGACY.
#------------------------------------------------------------------------------
option(DB_UNSPENT_LEGACY "DB Unspent Legacy enabled." ON)
if (DB_UNSPENT_LEGACY)
  message(STATUS "Knuth: DB Unspent Legacy enabled")
  add_definitions(-DKTH_DB_UNSPENT_LEGACY)
endif()

# Implement and declare DB_LEGACY.
#------------------------------------------------------------------------------
option(DB_LEGACY "DB Legacy enabled." ON)
if (DB_LEGACY)
  message(STATUS "Knuth: DB Legacy enabled")
  add_definitions(-DKTH_DB_LEGACY)
endif()

# Implement and declare DB_NEW.
#------------------------------------------------------------------------------
option(DB_NEW "DB New enabled." OFF)
if (DB_NEW)
  message(STATUS "Knuth: DB New enabled")
  add_definitions(-DKTH_DB_NEW)
endif()

# Implement and declare DB_NEW_BLOCKS.
#------------------------------------------------------------------------------
option(DB_NEW_BLOCKS "DB Blocks New enabled." OFF)
if (DB_NEW_BLOCKS)
  message(STATUS "Knuth: DB Blocks New enabled")
  add_definitions(-DKTH_DB_NEW_BLOCKS)
endif()


# Implement and declare DB_NEW_FULL.
#------------------------------------------------------------------------------
option(DB_NEW_FULL "DB New Full enabled." OFF)
if (DB_NEW_FULL)
  message(STATUS "Knuth: DB New Full enabled")
  add_definitions(-DKTH_DB_NEW_FULL)
endif()

# Implement --db_readonly_mode and declare KTH_DB_READONLY.
#------------------------------------------------------------------------------
option(DB_READONLY_MODE "Readonly DB mode enabled." OFF)
if (DB_READONLY_MODE)
  message(STATUS "Knuth: Readonly DB mode enabled")
  add_definitions(-DKTH_DB_READONLY)
endif()

# Implement --JUST_KTH_SOURCES for linting.
#------------------------------------------------------------------------------
option(JUST_KTH_SOURCES "Just Knuth source code to be linted." OFF)
if (JUST_KTH_SOURCES)
  message(STATUS "Knuth: Just Knuth source code to be linted: enabled")
endif()

# # Implement --binlog.
# #------------------------------------------------------------------------------
# option(BINLOG "Uses BinLog logging library." OFF)
# if (BINLOG)
#   add_definitions(-DKTH_USE_BINLOG)
# endif()

set(LOG_LIBRARY "boost" CACHE STRING "Setting for the logging library (boost|spdlog|binlog).")

if (${LOG_LIBRARY} STREQUAL "boost")
  add_definitions(-DKTH_LOG_LIBRARY_BOOST)
elseif (${LOG_LIBRARY} STREQUAL "spdlog")
  add_definitions(-DKTH_LOG_LIBRARY_SPDLOG)
elseif (${LOG_LIBRARY} STREQUAL "binlog")
  add_definitions(-DKTH_LOG_LIBRARY_BINLOG)
else()
  message(FATAL_ERROR "Invalid Logging Library: ${LOG_LIBRARY}")
endif()

message(STATUS "Knuth: Logging Library: ${LOG_LIBRARY}")


# Implement --use_libmdbx.
#------------------------------------------------------------------------------
option(USE_LIBMDBX "Uses libmdbx DB library." OFF)
if (USE_LIBMDBX)
  add_definitions(-DKTH_USE_LIBMDBX)
endif()



set(KTH_PROJECT_VERSION "-" CACHE STRING "Specify the Knuth Project Version.")
# message(${KTH_PROJECT_VERSION})
message( STATUS "Knuth: Compiling version ${KTH_PROJECT_VERSION}")

set(MICROARCHITECTURE "x86_64" CACHE STRING "Specify the Microarchitecture (x86_64|...).")
message( STATUS "Knuth: Compiling for ${MICROARCHITECTURE}")
# add_definitions(-DKTH_MICROARCHITECTURE=${MICROARCHITECTURE})
# add_definitions(-DKTH_MICROARCHITECTURE_STR="${MICROARCHITECTURE}")

set(MARCH_ID "" CACHE STRING "Specify the Microarchitecture ID (x86_64|...).")
message( STATUS "Knuth: Compiling for Microarchitecture ID ${MARCH_ID}")

message( STATUS "Knuth: CONAN_CXX_FLAGS ${CONAN_CXX_FLAGS}")
message( STATUS "Knuth: CONAN_C_FLAGS ${CONAN_C_FLAGS}")


set(CURRENCY "BCH" CACHE STRING "Specify the Cryptocurrency (BCH|BTC|LTC).")

if (${CURRENCY} STREQUAL "BCH")
  add_definitions(-DKTH_CURRENCY_BCH)
elseif (${CURRENCY} STREQUAL "BTC")
  add_definitions(-DKTH_CURRENCY_BTC)
elseif (${CURRENCY} STREQUAL "LTC")
  add_definitions(-DKTH_CURRENCY_LTC)
else()
  message(FATAL_ERROR "Invalid Cryptocurrency: ${CURRENCY}")
endif()

message(STATUS "Knuth: Cryptocurrency: ${CURRENCY}")


# Conan infrastructure.
#==============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/ci_utils/cmake/knuthbuildinfo.cmake)
    include(${CMAKE_SOURCE_DIR}/ci_utils/cmake/knuthbuildinfo.cmake)
else()
    message( STATUS "knuthbuildinfo doent exists")
endif()



# Check dependencies.
#==============================================================================
#------------------------------------------------------------------------------
if (NO_CONAN_AT_ALL)
  # Require Boost of at least version 1.56.0 and output ${boost_CPPFLAGS/LDFLAGS}.
  #------------------------------------------------------------------------------
  if (NOT ENABLE_SHARED)
    set(Boost_USE_STATIC_LIBS ON)
  endif()

  if (MSVC)
    find_package(
      Boost 1.66.0 REQUIRED
      COMPONENTS atomic chrono date_time filesystem iostreams locale log log_setup program_options system thread unit_test_framework
      # regex
    )
  else()
    find_package(
      Boost 1.66.0 REQUIRED
      COMPONENTS chrono date_time filesystem iostreams locale log program_options system thread unit_test_framework
      # regex
    )
  endif()
endif()

if (NOT USE_CONAN)
  # Require kth-database of at least version 0 and output ${kth_database_CPPFLAGS/LIBS/PKG}.
  #------------------------------------------------------------------------------
  if (NOT TARGET kth-database)
    find_package(kth-database 0 REQUIRED)
  endif()

  # Require kth-consensus of at least version 0 and output ${kth_consensus_CPPFLAGS/LIBS/PKG}.
  #------------------------------------------------------------------------------
  if (WITH_CONSENSUS)
    if (NOT TARGET kth-consensus)
      find_package(kth-consensus 0 REQUIRED)
    endif()
  endif()  
endif(NOT USE_CONAN)



# Set flags.
#==============================================================================
include(CheckCXXCompilerFlag)

# Including common functions
#------------------------------------------------------------------------------
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/ci_utils/cmake)

include(KnuthTools)


# Warn on all stuff.
#------------------------------------------------------------------------------
if (NOT MSVC)
  _add_c_compile_flag(-Wall _has_all_warning_flag)
else()
  _add_c_compile_flag(-W4 _has_all_warning_flag)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Warn on extra stuff.
#------------------------------------------------------------------------------
if (NOT MSVC)
  _add_c_compile_flag(-Wextra _has_extra_warning_flag)
endif()

# Be really annoying.
#------------------------------------------------------------------------------
_add_c_compile_flag(-Wpedantic _has_pedantic_warning_flag)
if (_has_pedantic_warning_flag)
  _add_c_compile_flag(-pedantic _has_pedantic_flag)
endif()

# Conform to style.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-Wno-missing-braces _has_no_missing_braces_warning_flag)

# Conflict in stdlib under clang. Enabled in clang only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-Wno-mismatched-tags _has_no_mismatched_tags_warning_flag)

# Clean up boost 1.55 headers. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_c_compile_flag(-Wno-deprecated-declarations _has_no_deprecated_declarations_warning_flag)

# Protect stack.
#------------------------------------------------------------------------------
_add_link_flag(-fstack-protector _has_stack_protector_flag)

# Protect stack comprehensively.
#------------------------------------------------------------------------------
_add_link_flag(-fstack-protector-all _has_stack_protector_all_flag)

# Hide internal functions from external libs. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-fvisibility-hidden _has_visibility_hidden_flag)

# Hide inlines from external libs. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-fvisibility-inlines-hidden _has_visibility_inlines_hidden_flag)

# Target Windows Vista. Enabled in msvc only.
#------------------------------------------------------------------------------
if (MSVC) # OR MINGW)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()

# Build
#==============================================================================

# src/kth-blockchain.la => ${libdir}
#------------------------------------------------------------------------------
set(MODE STATIC)
if (ENABLE_SHARED)
  set(MODE SHARED)
endif()

set(kth_blockchain_sources_just_legacy
  src/interface/block_chain.cpp

  src/pools/block_entry.cpp
  src/pools/block_organizer.cpp
  src/pools/block_pool.cpp
  src/pools/branch.cpp
  src/pools/transaction_entry.cpp
  src/pools/transaction_organizer.cpp
  src/pools/transaction_pool.cpp
  src/pools/mempool_transaction_summary.cpp #Rama

  src/populate/populate_base.cpp
  src/populate/populate_block.cpp
  src/populate/populate_chain_state.cpp
  src/populate/populate_transaction.cpp
  src/validate/validate_block.cpp
  src/validate/validate_input.cpp
  src/validate/validate_transaction.cpp
  src/settings.cpp
  src/version.cpp

  src/interface/block_chain_old_db.cpp
)

set(kth_blockchain_sources_just_kth
  src/interface/block_chain_old_db.cpp
)

# message(STATUS "TODO(fernando) WITH_KEOKEN: ${WITH_KEOKEN}")

if (WITH_KEOKEN)
  set(kth_blockchain_sources_just_kth 
    ${kth_blockchain_sources_just_kth}         
    src/keoken/memory_state.cpp
  )
endif()

if (NOT JUST_KTH_SOURCES)
  set(kth_blockchain_sources 
    ${kth_blockchain_sources_just_legacy}
  )
endif()

set(kth_blockchain_sources 
  ${kth_blockchain_sources}   
  ${kth_blockchain_sources_just_kth}         
)

add_library(kth-blockchain ${MODE} ${kth_blockchain_sources})



if (ENABLE_POSITION_INDEPENDENT_CODE)
    set_property(TARGET kth-blockchain PROPERTY POSITION_INDEPENDENT_CODE ON)
endif(ENABLE_POSITION_INDEPENDENT_CODE)
  
target_include_directories(kth-blockchain PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# target_compile_definitions(kth-blockchain PUBLIC -DKTH_PROJECT_VERSION="\\"${KTH_PROJECT_VERSION}\\"") #TODO(fernando): manage with Conan????  
target_compile_definitions(kth-blockchain PUBLIC -DKTH_PROJECT_VERSION="${KTH_PROJECT_VERSION}") #TODO(fernando): manage with Conan????


if (NOT ENABLE_SHARED)
  target_compile_definitions(kth-blockchain PUBLIC -DBCB_STATIC -DBCD_STATIC -DBC_STATIC)
endif()

if (WITH_CONSENSUS)
  target_compile_definitions(kth-blockchain PUBLIC -DBCK_STATIC) #TODO(fernando): por que esta afuera?
endif(WITH_CONSENSUS)

if (NOT USE_CONAN)
  target_link_libraries(kth-blockchain PUBLIC kth-database)
endif(NOT USE_CONAN)


# #TODO(fernando): See what to do with this option. Move to Conan?
# if (NOT NO_CONAN_AT_ALL)
#   if (WITH_CONSENSUS)
#     target_compile_definitions(kth-blockchain PUBLIC -DWITH_CONSENSUS)
#     target_link_libraries(kth-blockchain PUBLIC ${CONAN_LIBS})
#   else()
#     target_link_libraries(kth-blockchain PUBLIC ${CONAN_LIBS})
#   endif()
# else()
#   if (WITH_CONSENSUS)
#     target_link_libraries(kth-blockchain kth-consensus -DWITH_CONSENSUS)
#   else()
#     target_link_libraries(kth-blockchain kth-consensus)
#   endif()
# endif()



if (WITH_CONSENSUS)		 
  target_compile_definitions(kth-blockchain PUBLIC -DWITH_CONSENSUS)
  if (NOT USE_CONAN)		
    target_link_libraries(kth-blockchain PUBLIC kth-consensus)
  endif(NOT USE_CONAN)		 
endif(WITH_CONSENSUS)		 

if (NOT NO_CONAN_AT_ALL) # at least with external libraries
  target_link_libraries(kth-blockchain PUBLIC ${CONAN_LIBS})
endif()


# if (USE_CONAN)
  if (MINGW)
    target_link_libraries(kth-blockchain PUBLIC ws2_32 wsock32) #TODO(fernando): manage with Conan
  endif()
# endif()

_group_sources(kth-blockchain "${CMAKE_CURRENT_LIST_DIR}")

# Tests
#==============================================================================

if (WITH_TESTS OR WITH_TESTS_NEW)
  enable_testing()
endif()

# local: test/kth_blockchain_test
#------------------------------------------------------------------------------
if (WITH_TESTS)
  add_executable(kth_blockchain_test
    test/block_chain.cpp
    test/block_entry.cpp
    test/block_pool.cpp
    test/branch.cpp
    test/transaction_entry.cpp
    test/transaction_pool.cpp
    test/validate_block.cpp
    test/validate_transaction.cpp
    test/utxo.cpp
    test/main.cpp
  )

  target_link_libraries(kth_blockchain_test PUBLIC kth-blockchain)
  _group_sources(kth_blockchain_test "${CMAKE_CURRENT_LIST_DIR}/test")

  # _add_tests(kth_blockchain_test "blockchain" transaction_pool_tests) # validate_block_tests) # no test cases

  _add_tests(kth_blockchain_test 
    block_entry_tests
    block_pool_tests
    branch_tests
    transaction_entry_tests
    validate_block_tests
    validate_transaction_tests
  )

  if (DB_LEGACY)
    _add_tests(kth_blockchain_test 
      fast_chain_tests
      safe_chain_tests
    )
  endif()

  if (DB_NEW)
    if (NOT WIN32)
      _add_tests(kth_blockchain_test 
        utxo_tests
      )
    endif()
  endif()
endif()

if (WITH_TESTS_NEW)
    if (WITH_MEMPOOL OR WITH_KEOKEN)
        set(kth_blockchain_test_new_sources 
            test_new/main.cpp
        )

        if (WITH_MEMPOOL)
            set(kth_blockchain_test_new_sources 
                ${kth_blockchain_test_new_sources}
                test_new/mempool_tests.cpp
            )
        endif()

        if (WITH_KEOKEN)
            set(kth_blockchain_test_new_sources 
                ${kth_blockchain_test_new_sources}   
                test_new/state_delegated_tests.cpp
                test_new/state_tests.cpp
                test_new/interpreter_tests.cpp
            )
        endif()

        add_executable(kth_blockchain_test_new
            ${kth_blockchain_test_new_sources}
        )
        target_link_libraries(kth_blockchain_test_new PUBLIC kth-blockchain)
        add_test(NAME kth_blockchain_test_new COMMAND kth_blockchain_test_new)
    endif()
endif()


# Tools
#==============================================================================
# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if (WITH_TOOLS)
  add_executable(tools.initchain tools/initchain/initchain.cpp)

  target_link_libraries(tools.initchain kth-blockchain)
  _group_sources(tools.initchain "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# Install
#==============================================================================
install(TARGETS kth-blockchain EXPORT kth-blockchain
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

set(_kth_headers
  # include_kth_HEADERS =
 kth/blockchain.hpp
  # include_kth_blockchain_HEADERS =
 kth/blockchain/define.hpp
 kth/blockchain/settings.hpp
 kth/blockchain/version.hpp
  # include_kth_blockchain_interface_HEADERS =
 kth/blockchain/interface/block_chain.hpp
  #kth/blockchain/interface/block_fetcher.hpp
 kth/blockchain/interface/fast_chain.hpp
 kth/blockchain/interface/safe_chain.hpp
  # include_kth_blockchain_pools_HEADERS =
 kth/blockchain/pools/block_entry.hpp
 kth/blockchain/pools/block_organizer.hpp
 kth/blockchain/pools/block_pool.hpp
 kth/blockchain/pools/branch.hpp
 kth/blockchain/pools/transaction_entry.hpp
 kth/blockchain/pools/transaction_organizer.hpp
 kth/blockchain/pools/transaction_pool.hpp
  
  #Rama
 kth/blockchain/pools/mempool_transaction_summary.hpp 

  # include_kth_blockchain_populate_HEADERS =
 kth/blockchain/populate/populate_base.hpp
 kth/blockchain/populate/populate_block.hpp
 kth/blockchain/populate/populate_chain_state.hpp
 kth/blockchain/populate/populate_transaction.hpp
  # include_kth_blockchain_validation_HEADERS =
 kth/blockchain/validate/validate_block.hpp
 kth/blockchain/validate/validate_input.hpp
 kth/blockchain/validate/validate_transaction.hpp
)
  
foreach (_header ${_kth_headers})
  get_filename_component(_directory "${_header}" DIRECTORY)
  install(FILES "include/${_header}" DESTINATION "include/${_directory}")
endforeach()

# Export
#==============================================================================
include(CMakePackageConfigHelpers)

export(EXPORT kth-blockchain
  FILE "${CMAKE_CURRENT_BINARY_DIR}/kth-blockchainTargets.cmake")
export(PACKAGE kth-blockchain)

if (UNIX)
  set(_config_package_location "lib/cmake")
elseif (WIN32)
  set(_config_package_location "cmake")
endif()

install(EXPORT kth-blockchain
  FILE kth-blockchainTargets.cmake
  DESTINATION ${_config_package_location})

configure_file(kth-blockchainConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/kth-blockchainConfig.cmake"
  @ONLY)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/kth-blockchainConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kth-blockchainConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kth-blockchainConfigVersion.cmake"
  DESTINATION ${_config_package_location})
