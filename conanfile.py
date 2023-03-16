# Copyright (c) 2016-2023 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
<<<<<<< Updated upstream
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
=======
from conan import ConanFile
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy #, apply_conandata_patches, export_conandata_patches, get, rm, rmdir
>>>>>>> Stashed changes
from kthbuild import option_on_off, march_conan_manip, pass_march_to_compiler
from kthbuild import KnuthConanFileV2

required_conan_version = ">=2.0"


class KnuthBlockchainConan(KnuthConanFileV2):
    name = "blockchain"
    license = "http://www.boost.org/users/license.html"
    url = "https://github.com/k-nuth/blockchain/blob/conan-build/conanfile.py"
    description = "Knuth Blockchain Library"
    settings = "os", "compiler", "build_type", "arch"

    options = {"shared": [True, False],
               "fPIC": [True, False],
               "consensus": [True, False],
               "tests": [True, False],
               "tools": [True, False],
               "currency": ['BCH', 'BTC', 'LTC'],

               "march_id": ["ANY"],
               "march_strategy": ["download_if_possible", "optimized", "download_or_fail"],

               "verbose": [True, False],
            #    "mining": [True, False],
               "mempool": [True, False],
               "db": ['legacy', 'legacy_full', 'pruned', 'default', 'full'],
               "db_readonly": [True, False],

               "cxxflags": ["ANY"],
               "cflags": ["ANY"],
               "cmake_export_compile_commands": [True, False],
               "log": ["boost", "spdlog", "binlog"],
               "use_libmdbx": [True, False],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "consensus": True,
        "tests": False,
        "tools": False,
        "currency": "BCH",

        "march_strategy": "download_if_possible",

        "verbose": False,

        "mempool": False,
        "db": "default",
        "db_readonly": False,

        "cmake_export_compile_commands": False,
        "log": "spdlog",
        "use_libmdbx": False,
    }
    # "mining=False", \

    # generators = "cmake"
<<<<<<< Updated upstream
    exports = "conan_*", "ci_utils/*"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "kth-blockchainConfig.cmake.in", "knuthbuildinfo.cmake", "include/*", "test/*", "tools/*"
=======
    # exports = "conan_*", "ci_utils/*"
    exports_sources = "src/*", "CMakeLists.txt", "ci_utils/cmake/*", "cmake/*", "kth-blockchainConfig.cmake.in", "knuthbuildinfo.cmake", "include/*", "test/*", "tools/*"
>>>>>>> Stashed changes
    package_files = "build/lkth-blockchain.a"
    # build_policy = "missing"

    def build_requirements(self):
        if self.options.tests:
            self.test_requires("catch2/3.3.1")

    def requirements(self):
        self.requires("database/0.X@%s/%s" % (self.user, self.channel))

        if self.options.consensus:
            self.requires.add("consensus/0.X@%s/%s" % (self.user, self.channel))

    def validate(self):
<<<<<<< Updated upstream
        KnuthConanFile.validate(self)
        if self.info.settings.compiler.cppstd:
            check_min_cppstd(self, "20")

=======
        KnuthConanFileV2.validate(self)
>>>>>>> Stashed changes

    def config_options(self):
        KnuthConanFileV2.config_options(self)

    def configure(self):
        KnuthConanFileV2.configure(self)

        self.options["*"].db_readonly = self.options.db_readonly
        self.output.info("Compiling with read-only DB: %s" % (self.options.db_readonly,))

        # self.options["*"].mining = self.options.mining
        # self.options["*"].mempool = self.options.mempool
        # self.output.info("Compiling with mining optimizations: %s" % (self.options.mining,))
        self.output.info("Compiling with mempool: %s" % (self.options.mempool,))

        #TODO(fernando): move to kthbuild
        self.options["*"].log = self.options.log
        self.output.info("Compiling with log: %s" % (self.options.log,))

        self.options["*"].use_libmdbx = self.options.use_libmdbx
        self.output.info("Compiling with use_libmdbx: %s" % (self.options.use_libmdbx,))


    def package_id(self):
        KnuthConanFileV2.package_id(self)
        self.info.options.tools = "ANY"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = self.cmake_toolchain_basis()
        # tc.variables["CMAKE_VERBOSE_MAKEFILE"] = True
        tc.variables["WITH_CONSENSUS"] = option_on_off(self.options.consensus)
<<<<<<< Updated upstream

=======
        # tc.variables["WITH_KEOKEN"] = option_on_off(self.is_keoken)
        tc.variables["WITH_KEOKEN"] = option_on_off(False)
>>>>>>> Stashed changes
        # tc.variables["WITH_MINING"] = option_on_off(self.options.mining)
        tc.variables["WITH_MEMPOOL"] = option_on_off(self.options.mempool)
        tc.variables["DB_READONLY_MODE"] = option_on_off(self.options.db_readonly)
        tc.variables["LOG_LIBRARY"] = self.options.log
        tc.variables["USE_LIBMDBX"] = option_on_off(self.options.use_libmdbx)
        tc.variables["CONAN_DISABLE_CHECK_COMPILER"] = option_on_off(True)

        tc.generate()
        tc = CMakeDeps(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
        if not self.options.cmake_export_compile_commands:
            cmake.build()
            if self.options.tests:
                cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        # rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        # rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        # rmdir(self, os.path.join(self.package_folder, "res"))
        # rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libs = ["kth-blockchain"]
