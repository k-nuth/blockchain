#
# Copyright (c) 2017 Bitprim developers (see AUTHORS)
#
# This file is part of Bitprim.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.4)

# bitprim-blockchain
#==============================================================================
project(bitprim-blockchain
  VERSION 0.3.0
  LANGUAGES CXX C)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (NOT NOT_USE_CPP11_ABI)
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
  else()
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
  endif()
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-macro-redefined")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check for baseline language coverage in the compiler for the C++11 standard.
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 11)
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
option(WITH_CONSENSUS "Link bitprim-consensus and use for consensus checks." ON)

# Implement --with-tests and declare WITH_TESTS.
#------------------------------------------------------------------------------
option(WITH_TESTS "Compile with unit tests." ON)

# Implement --with-tools and declare WITH_TOOLS.
#------------------------------------------------------------------------------
option(WITH_TOOLS "Compile with tools." OFF)

# Implement --with-litecoin.
#------------------------------------------------------------------------------
option(WITH_LITECOIN "Compile with Litecoin support." OFF)




if (WITH_LITECOIN)
  add_definitions(-DLITECOIN)
endif()


# Implement --with-remote-database and declare WITH_REMOTE_DATABASE.
#------------------------------------------------------------------------------
option(WITH_REMOTE_DATABASE "Compile with remote bitprim-database (aka. bitprim-database-requester)." OFF)

# Conan infrastructure.
#==============================================================================
# Only if enabled
#------------------------------------------------------------------------------
if (NOT NO_CONAN_AT_ALL)
  if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
      include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
      conan_basic_setup()
  else()
      message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
  endif()
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
      Boost 1.56.0 REQUIRED
      COMPONENTS atomic chrono date_time filesystem iostreams locale log log_setup program_options regex system thread unit_test_framework
    )
  else()
    find_package(
      Boost 1.56.0 REQUIRED
      COMPONENTS chrono date_time filesystem iostreams locale log program_options regex system thread unit_test_framework
    )
  endif()
endif()

if (NOT USE_CONAN)
  # Require bitprim-database (or bitprim-database-requester) of at least version 0.3.0 and output ${bitprim_database_CPPFLAGS/LIBS/PKG}.
  #------------------------------------------------------------------------------
  if (WITH_REMOTE_DATABASE)
    if (NOT TARGET bitprim-database-requester)
      find_package(bitprim-database-requester 0.3.0 REQUIRED)
    endif()
    else()
    if (NOT TARGET bitprim-database)
      find_package(bitprim-database 0.3.0 REQUIRED)
    endif()
  endif()

  # Require bitprim-consensus of at least version 0.3.0 and output ${bitprim_consensus_CPPFLAGS/LIBS/PKG}.
  #------------------------------------------------------------------------------
  if (WITH_CONSENSUS)
    if (NOT TARGET bitprim-consensus)
      find_package(bitprim-consensus 0.3.0 REQUIRED)
    endif()
  endif()  
endif(NOT USE_CONAN)



# Set flags.
#==============================================================================
include(CheckCXXCompilerFlag)

# Including common functions
#------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(BitprimTools)


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
if (MSVC)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()

# Build
#==============================================================================

# src/bitprim-blockchain.la => ${libdir}
#------------------------------------------------------------------------------
set(MODE STATIC)
if (ENABLE_SHARED)
  set(MODE SHARED)
endif()

add_library(bitprim-blockchain ${MODE}
  src/interface/block_chain.cpp
  src/pools/block_entry.cpp
  src/pools/block_organizer.cpp
  src/pools/block_pool.cpp
  src/pools/branch.cpp
  src/pools/transaction_entry.cpp
  src/pools/transaction_organizer.cpp
  src/pools/transaction_pool.cpp
  src/populate/populate_base.cpp
  src/populate/populate_block.cpp
  src/populate/populate_chain_state.cpp
  src/populate/populate_transaction.cpp
  src/validate/validate_block.cpp
  src/validate/validate_input.cpp
  src/validate/validate_transaction.cpp
  src/settings.cpp
)

if (ENABLE_POSITION_INDEPENDENT_CODE)
  set_property(TARGET bitprim-blockchain PROPERTY POSITION_INDEPENDENT_CODE ON)
endif(ENABLE_POSITION_INDEPENDENT_CODE)
  
target_include_directories(bitprim-blockchain PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)


if (NOT ENABLE_SHARED)
  target_compile_definitions(bitprim-blockchain PUBLIC -DBCB_STATIC -DBCD_STATIC -DBC_STATIC)
endif()

if (WITH_CONSENSUS)
  target_compile_definitions(bitprim-blockchain PUBLIC -DBCK_STATIC) #TODO(fernando): por que esta afuera?
endif(WITH_CONSENSUS)

if (WITH_REMOTE_DATABASE)
  if (NOT USE_CONAN)
    target_link_libraries(bitprim-blockchain bitprim-database-requester)
  endif(NOT USE_CONAN)

  target_compile_definitions(bitprim-blockchain PUBLIC -DWITH_REMOTE_DATABASE)
else()
  if (NOT USE_CONAN)
    target_link_libraries(bitprim-blockchain PUBLIC bitprim-database)
  endif(NOT USE_CONAN)
endif()




# #TODO(fernando): See what to do with this option. Move to Conan?
# if (NOT NO_CONAN_AT_ALL)
#   if (WITH_CONSENSUS)
#     target_compile_definitions(bitprim-blockchain PUBLIC -DWITH_CONSENSUS)
#     target_link_libraries(bitprim-blockchain PUBLIC ${CONAN_LIBS})
#   else()
#     target_link_libraries(bitprim-blockchain PUBLIC ${CONAN_LIBS})
#   endif()
# else()
#   if (WITH_CONSENSUS)
#     target_link_libraries(bitprim-blockchain bitprim-consensus -DWITH_CONSENSUS)
#   else()
#     target_link_libraries(bitprim-blockchain bitprim-consensus)
#   endif()
# endif()



if (WITH_CONSENSUS)		 
  target_compile_definitions(bitprim-blockchain PUBLIC -DWITH_CONSENSUS)
  if (NOT USE_CONAN)		
    target_link_libraries(bitprim-blockchain PUBLIC bitprim-consensus)
  endif(NOT USE_CONAN)		 
endif(WITH_CONSENSUS)		 

if (NOT NO_CONAN_AT_ALL) # at least with external libraries
  target_link_libraries(bitprim-blockchain PUBLIC ${CONAN_LIBS})
endif()


if (USE_CONAN)
  if (MINGW)
    target_link_libraries(bitprim-blockchain PUBLIC ws2_32 wsock32) #TODO(fernando): manage with Conan
  endif()
endif()

_group_sources(bitprim-blockchain "${CMAKE_CURRENT_LIST_DIR}")

# # src/bitprim-blockchain-replier => ${bindir}
# #------------------------------------------------------------------------------
# add_executable(bitprim-blockchain-replier
#   replier/src/blockchain.cpp
#   replier/src/configuration.cpp
#   replier/src/main.cpp
#   replier/src/parser.cpp)
# if (NOT MSVC)
#   target_compile_definitions(bitprim-blockchain-replier PUBLIC -DSYSCONFDIR=\"${SYSCONFDIR}\")
# endif()
# target_link_libraries(bitprim-blockchain-replier bitprim-network bitprim-protocol bitprim-blockchain)
# target_include_directories(bitprim-blockchain-replier PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/replier/include)

# _group_sources(bitprim-blockchain-replier "${CMAKE_CURRENT_LIST_DIR}/replier")

# # src/bitprim-blockchain-requester.la => ${libdir}
# #------------------------------------------------------------------------------
# add_library(bitprim-blockchain-requester ${MODE}
#   requester/src/block_chain.cpp
#   src/interface/block_fetcher.cpp
#   src/pools/orphan_pool.cpp
#   src/pools/orphan_pool_manager.cpp
#   src/pools/transaction_pool.cpp
#   src/pools/transaction_pool_index.cpp
#   src/settings.cpp
#   src/validation/fork.cpp
#   src/validation/populate_block.cpp
#   src/validation/validate_block.cpp
#   src/validation/validate_input.cpp)
# target_include_directories(bitprim-blockchain-requester PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#   $<INSTALL_INTERFACE:include>)
# 
# target_compile_definitions(bitprim-blockchain-requester PUBLIC -DBCB_STATIC)
# target_include_directories(bitprim-blockchain-requester PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/src/clone)

# target_compile_definitions(bitprim-blockchain-requester PUBLIC
#   -DWITH_BLOCKCHAIN_REQUESTER)
# add_dependencies(bitprim-blockchain-requester bitprim-blockchain-replier)
# 
# target_link_libraries(bitprim-blockchain-requester bitprim-database bitprim-protocol)
# if (WITH_CONSENSUS)
#   target_link_libraries(bitprim-blockchain-requester bitprim-consensus-requester)
# endif()

# _group_sources(bitprim-blockchain-requester "${CMAKE_CURRENT_LIST_DIR}/requester")

# Tests
#==============================================================================

if (WITH_TESTS)
  enable_testing()
endif()

# local: test/bitprim_blockchain_test
#------------------------------------------------------------------------------
if (WITH_TESTS)
  add_executable(bitprim_blockchain_test
    test/main.cpp
    test/transaction_pool.cpp
    test/validate_block.cpp)

  target_link_libraries(bitprim_blockchain_test PUBLIC bitprim-blockchain)
  _group_sources(bitprim_blockchain_test "${CMAKE_CURRENT_LIST_DIR}/test")

  _add_tests(bitprim_blockchain_test "blockchain" transaction_pool_tests) # validate_block_tests) # no test cases
endif()

# # local: test/bitprim_blockchain_requester_test
# #------------------------------------------------------------------------------
# if (WITH_TESTS)
#   add_executable(bitprim_blockchain_requester_test
#     test/main.cpp
#     test/transaction_pool.cpp
#     test/validate_block.cpp)
#   target_compile_definitions(bitprim_blockchain_requester_test PRIVATE
#     -DWITH_BLOCKCHAIN_REPLIER="$<TARGET_FILE:bitprim-blockchain-replier>")
#   target_include_directories(bitprim_blockchain_requester_test PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/requester/test)
#   target_link_libraries(bitprim_blockchain_requester_test PUBLIC bitprim-blockchain-requester)
#   if (WITH_CONSENSUS)
#     target_compile_definitions(bitprim_blockchain_requester_test PRIVATE
#       -DWITH_CONSENSUS_REPLIER="$<TARGET_FILE:bitprim-consensus-replier>")
#   endif()

#   _add_tests(bitprim_blockchain_requester_test "blockchain-requester"
#     transaction_pool_tests)
#     # validate_block_tests) # no test cases

#   _group_sources(bitprim_blockchain_requester_test "${CMAKE_CURRENT_LIST_DIR}/test")
# endif()

# Tools
#==============================================================================
# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if (WITH_TOOLS)
  add_executable(tools.initchain tools/initchain/initchain.cpp)
  target_link_libraries(tools.initchain bitprim-blockchain)
  _group_sources(tools.initchain "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# Install
#==============================================================================
# install(TARGETS bitprim-blockchain bitprim-blockchain-requester bitprim-blockchain-replier
install(TARGETS bitprim-blockchain
  EXPORT bitprim-blockchain
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

set(_bitprim_headers
  # include_bitcoin_HEADERS =
  bitcoin/blockchain.hpp
  # include_bitcoin_blockchain_HEADERS =
  bitcoin/blockchain/define.hpp
  bitcoin/blockchain/settings.hpp
  bitcoin/blockchain/version.hpp
  # include_bitcoin_blockchain_interface_HEADERS =
  bitcoin/blockchain/interface/block_chain.hpp
  #bitcoin/blockchain/interface/block_fetcher.hpp
  bitcoin/blockchain/interface/fast_chain.hpp
  bitcoin/blockchain/interface/safe_chain.hpp
  # include_bitcoin_blockchain_pools_HEADERS =
  bitcoin/blockchain/pools/block_entry.hpp
  bitcoin/blockchain/pools/block_organizer.hpp
  bitcoin/blockchain/pools/block_pool.hpp
  bitcoin/blockchain/pools/branch.hpp
  bitcoin/blockchain/pools/transaction_entry.hpp
  bitcoin/blockchain/pools/transaction_organizer.hpp
  bitcoin/blockchain/pools/transaction_pool.hpp
  # include_bitcoin_blockchain_populate_HEADERS =
  bitcoin/blockchain/populate/populate_base.hpp
  bitcoin/blockchain/populate/populate_block.hpp
  bitcoin/blockchain/populate/populate_chain_state.hpp
  bitcoin/blockchain/populate/populate_transaction.hpp
  # include_bitcoin_blockchain_validation_HEADERS =
  bitcoin/blockchain/validate/validate_block.hpp
  bitcoin/blockchain/validate/validate_input.hpp
  bitcoin/blockchain/validate/validate_transaction.hpp)
foreach (_header ${_bitprim_headers})
  get_filename_component(_directory "${_header}" DIRECTORY)
  install(FILES "include/${_header}" DESTINATION "include/${_directory}")
endforeach()

# Export
#==============================================================================
include(CMakePackageConfigHelpers)

export(EXPORT bitprim-blockchain
  FILE "${CMAKE_CURRENT_BINARY_DIR}/bitprim-blockchainTargets.cmake")
export(PACKAGE bitprim-blockchain)

if (UNIX)
  set(_config_package_location "lib/cmake")
elseif (WIN32)
  set(_config_package_location "cmake")
endif()

install(EXPORT bitprim-blockchain
  FILE bitprim-blockchainTargets.cmake
  DESTINATION ${_config_package_location})

configure_file(bitprim-blockchainConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/bitprim-blockchainConfig.cmake"
  @ONLY)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/bitprim-blockchainConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/bitprim-blockchainConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bitprim-blockchainConfigVersion.cmake"
  DESTINATION ${_config_package_location})
