cmake_minimum_required(VERSION 3.4)

# bitprim-blockchain
#==============================================================================
project(bitprim-blockchain
  VERSION 3.0.0
  LANGUAGES CXX C)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check for baseline language coverage in the compiler for the C++11 standard.
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Process options.
#==============================================================================
# Implement --with-consensus and define WITH_CONSENSUS.
#------------------------------------------------------------------------------
option(WITH_CONSENSUS "Link bitprim-consensus and use for consensus checks." ON)

# Implement --with-tests and declare WITH_TESTS.
#------------------------------------------------------------------------------
option(WITH_TESTS "Compile with unit tests." ON)

# Implement --with-tools and declare WITH_TOOLS.
#------------------------------------------------------------------------------
option(WITH_TOOLS "Compile with tools." ON)

# Inherit --enable-shared and define BOOST_TEST_DYN_LINK.
#------------------------------------------------------------------------------
option(ENABLE_SHARED "" OFF)

# Check dependencies.
#==============================================================================
# Require Boost of at least version 1.56.0 and output ${boost_CPPFLAGS/LDFLAGS}.
#------------------------------------------------------------------------------
if (NOT ENABLE_SHARED)
  set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(
  Boost 1.56.0 REQUIRED
  COMPONENTS unit_test_framework)

# Require bitprim-database of at least version 3.0.0 and output ${bitprim_database_CPPFLAGS/LIBS/PKG}.
#------------------------------------------------------------------------------
if (NOT TARGET bitprim-database)
  find_package(bitprim-database 3.0.0 REQUIRED)
endif()

# Require bitprim-protocol of at least version 3.0.0 and output ${bitprim_protocol_CPPFLAGS/LIBS/PKG}.
#------------------------------------------------------------------------------
if (NOT TARGET bitprim-protocol)
  find_package(bitprim-protocol 3.0.0 REQUIRED)
endif()

# Require bitprim-consensus of at least version 2.0.0 and output ${bitprim_consensus_CPPFLAGS/LIBS/PKG}.
#------------------------------------------------------------------------------
if (WITH_CONSENSUS)
  if (NOT TARGET bitprim-consensus)
    find_package(bitprim-consensus 2.0.0 REQUIRED)
  endif()
endif()

# Set flags.
#==============================================================================
include(CheckCXXCompilerFlag)

function(_add_c_compile_flag _Flag _Var)
  check_cxx_compiler_flag(${_Flag} ${_Var})
  if (${_Var})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_Flag}" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_Flag}" )
  endif()
endfunction()

function(_add_cxx_compile_flag _Flag _Var)
  check_cxx_compiler_flag(${_Flag} ${_Var})
  if (${_Var})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_Flag}" )
  endif()
endfunction()

function(_add_link_flag _Flag _Var)
  check_cxx_compiler_flag(${_Flag} ${_Var})
  if (${_Var})
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_Flag}" )
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${_Flag}" )
  endif()
endfunction()

# Warn on all stuff.
#------------------------------------------------------------------------------
if (NOT MSVC)
  _add_c_compile_flag(-Wall _has_all_warning_flag)
else()
  _add_c_compile_flag(-W4 _has_all_warning_flag)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
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
function(_group_sources target sources_dir)
  file(GLOB_RECURSE _headers
    ${sources_dir}/include/*.h ${sources_dir}/include/*.hpp)
  target_sources(${target} PRIVATE ${_headers})

  get_target_property(sources ${target} SOURCES)
  foreach (source ${sources})
    get_filename_component(group ${source} ABSOLUTE)
    get_filename_component(group ${group} DIRECTORY)
    file(RELATIVE_PATH group "${sources_dir}" "${group}")
    if (group)
      if (MSVC)
        string(REPLACE "/" "\\" group "${group}")
      endif()
      source_group("${group}" FILES "${source}")
    endif()
  endforeach()

  set_target_properties(${target} PROPERTIES FOLDER "blockchain")
endfunction()

# src/bitprim-blockchain.la => ${libdir}
#------------------------------------------------------------------------------
set(MODE STATIC)
if (ENABLE_SHARED)
  set(MODE SHARED)
endif()

add_library(bitprim-blockchain ${MODE}
  src/interface/block_chain.cpp
  src/interface/block_fetcher.cpp
  src/pools/orphan_pool.cpp
  src/pools/orphan_pool_manager.cpp
  src/pools/transaction_pool.cpp
  src/pools/transaction_pool_index.cpp
  src/settings.cpp
  src/validation/fork.cpp
  src/validation/populate_block.cpp
  src/validation/validate_block.cpp
  src/validation/validate_input.cpp)

target_include_directories(bitprim-blockchain PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(bitprim-blockchain PUBLIC -DBCB_STATIC)
if (WITH_CONSENSUS)
  target_compile_definitions(bitprim-blockchain PUBLIC -DWITH_CONSENSUS)
endif()

target_link_libraries(bitprim-blockchain bitprim-database)
if (WITH_CONSENSUS)
  target_link_libraries(bitprim-blockchain bitprim-consensus)
endif()

_group_sources(bitprim-blockchain "${CMAKE_CURRENT_LIST_DIR}")

# src/bitprim-blockchain-replier => ${bindir}
#------------------------------------------------------------------------------
add_executable(bitprim-blockchain-replier
  replier/src/blockchain.cpp
  replier/src/configuration.cpp
  replier/src/main.cpp
  replier/src/parser.cpp)
target_link_libraries(bitprim-blockchain-replier bitprim-protocol bitprim-blockchain)
target_include_directories(bitprim-blockchain-replier PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/replier/include)

_group_sources(bitprim-blockchain-replier "${CMAKE_CURRENT_LIST_DIR}/replier")

# src/bitprim-blockchain-requester.la => ${libdir}
#------------------------------------------------------------------------------
add_library(bitprim-blockchain-requester ${MODE}
  requester/src/block_chain.cpp
  src/interface/block_fetcher.cpp
  src/pools/orphan_pool.cpp
  src/pools/orphan_pool_manager.cpp
  src/pools/transaction_pool.cpp
  src/pools/transaction_pool_index.cpp
  src/settings.cpp
  src/validation/fork.cpp
  src/validation/populate_block.cpp
  src/validation/validate_block.cpp
  src/validation/validate_input.cpp)
target_include_directories(bitprim-blockchain-requester PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(bitprim-blockchain-requester PUBLIC -DBCB_STATIC)
target_include_directories(bitprim-blockchain-requester PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/clone)

target_compile_definitions(bitprim-blockchain-requester PUBLIC
  -DWITH_BLOCKCHAIN_REQUESTER)
add_dependencies(bitprim-blockchain-requester bitprim-blockchain-replier)

target_link_libraries(bitprim-blockchain-requester bitprim-database bitprim-protocol)
if (WITH_CONSENSUS)
  target_link_libraries(bitprim-blockchain-requester bitprim-consensus-requester)
endif()

_group_sources(bitprim-blockchain-requester "${CMAKE_CURRENT_LIST_DIR}/requester")

# Tests
#==============================================================================
function(_add_tests target group)
  if (ENABLE_SHARED)
    target_compile_definitions(${target} PRIVATE -DBOOST_TEST_DYN_LINK)
  endif()

  target_include_directories(${target} SYSTEM PUBLIC ${Boost_INCLUDE_DIR})
  target_link_libraries(${target} PUBLIC ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  foreach (_test_name ${ARGN})
    add_test(
      NAME test.${group}.${_test_name}
      COMMAND ${target}
        --run_test=${_test_name}
        --show_progress=no
        --detect_memory_leak=0
        --report_level=no
        --build_info=yes)
  endforeach()
endfunction()

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

  _add_tests(bitprim_blockchain_test "blockchain"
    transaction_pool_tests)
    # validate_block_tests) # no test cases
endif()

# local: test/bitprim_blockchain_requester_test
#------------------------------------------------------------------------------
if (WITH_TESTS)
  add_executable(bitprim_blockchain_requester_test
    test/main.cpp
    test/transaction_pool.cpp
    test/validate_block.cpp)
  target_compile_definitions(bitprim_blockchain_requester_test PRIVATE
    -DWITH_BLOCKCHAIN_REPLIER="$<TARGET_FILE:bitprim-blockchain-replier>")
  target_include_directories(bitprim_blockchain_requester_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/requester/test)
  target_link_libraries(bitprim_blockchain_requester_test PUBLIC bitprim-blockchain-requester)
  if (WITH_CONSENSUS)
    target_compile_definitions(bitprim_blockchain_requester_test PRIVATE
      -DWITH_CONSENSUS_REPLIER="$<TARGET_FILE:bitprim-consensus-replier>")
  endif()

  _add_tests(bitprim_blockchain_requester_test "blockchain-requester"
    transaction_pool_tests)
    # validate_block_tests) # no test cases

  _group_sources(bitprim_blockchain_requester_test "${CMAKE_CURRENT_LIST_DIR}/test")
endif()

# Tools
#==============================================================================
# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if (WITH_TOOLS)
  add_executable(tools.initchain
    tools/initchain/initchain.cpp)
  target_link_libraries(tools.initchain bitprim-blockchain)
  _group_sources(tools.initchain "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# Install
#==============================================================================
install(TARGETS bitprim-blockchain bitprim-blockchain-requester bitprim-blockchain-replier
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
  bitcoin/blockchain/interface/block_fetcher.hpp
  bitcoin/blockchain/interface/fast_chain.hpp
  bitcoin/blockchain/interface/safe_chain.hpp
  # include_bitcoin_blockchain_pools_HEADERS =
  bitcoin/blockchain/pools/orphan_pool.hpp
  bitcoin/blockchain/pools/orphan_pool_manager.hpp
  bitcoin/blockchain/pools/transaction_pool.hpp
  bitcoin/blockchain/pools/transaction_pool_index.hpp
  # include_bitcoin_blockchain_validation_HEADERS =
  bitcoin/blockchain/validation/fork.hpp
  bitcoin/blockchain/validation/populate_block.hpp
  bitcoin/blockchain/validation/validate_block.hpp
  bitcoin/blockchain/validation/validate_input.hpp)
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
