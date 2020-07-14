# Copyright (c) 2016-2020 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
from conans import CMake
from kthbuild import option_on_off, march_conan_manip, pass_march_to_compiler
from kthbuild import KnuthConanFile

class KnuthBlockchainConan(KnuthConanFile):
    def recipe_dir(self):
        return os.path.dirname(os.path.abspath(__file__))

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
               "microarchitecture": "ANY",
               "fix_march": [True, False],
               "march_id": "ANY",

               "verbose": [True, False],
               "keoken": [True, False],
            #    "mining": [True, False],
               "mempool": [True, False],
               "db": ['legacy', 'legacy_full', 'pruned', 'default', 'full'],
               "db_readonly": [True, False],

               "cxxflags": "ANY",
               "cflags": "ANY",
               "glibcxx_supports_cxx11_abi": "ANY",
               "cmake_export_compile_commands": [True, False],
               "log": ["boost", "spdlog", "binlog"],
               "use_libmdbx": [True, False],
    }
    # "with_remote_database": [True, False],

    default_options = {
        "shared": False,
        "fPIC": True,
        "consensus": True,
        "tests": False,
        "tools": False,
        "currency": "BCH",
        
        "microarchitecture": "_DUMMY_",
        "fix_march": False,
        "march_id": "_DUMMY_",
        
        "verbose": False,
        "keoken": False,
        "mempool": False,
        "db": "default",
        "db_readonly": False,

        "cxxflags": "_DUMMY_",
        "cflags": "_DUMMY_",
        "glibcxx_supports_cxx11_abi": "_DUMMY_",
        "cmake_export_compile_commands": False,
        "log": "boost",
        "use_libmdbx": False,
    }
        # "mining=False", \
    # "with_remote_database=False"

    generators = "cmake"
    exports = "conan_*", "ci_utils/*"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "kth-blockchainConfig.cmake.in", "knuthbuildinfo.cmake", "include/*", "test/*", "tools/*"
    package_files = "build/lkth-blockchain.a"
    build_policy = "missing"

    @property
    def is_keoken(self):
        return self.options.currency == "BCH" and self.options.get_safe("keoken")

    def requirements(self):
        self.requires("database/0.X@%s/%s" % (self.user, self.channel))

        if self.options.consensus:
            self.requires.add("consensus/0.X@%s/%s" % (self.user, self.channel))

        if self.options.tests:
            self.requires("catch2/2.12.2@")

    def config_options(self):
        KnuthConanFile.config_options(self)

        if self.options.keoken and self.options.currency != "BCH":
            self.output.warning("Keoken is only enabled for BCH, for the moment. Removing Keoken support")
            self.options.remove("keoken")


    def configure(self):
        KnuthConanFile.configure(self)

        if self.options.keoken and self.options.currency != "BCH":
            self.output.warn("For the moment Keoken is only enabled for BCH. Building without Keoken support...")
            del self.options.keoken
        else:
            self.options["*"].keoken = self.options.keoken

        if self.is_keoken:
            if self.options.db == "pruned" or self.options.db == "default":
                self.output.warn("Keoken mode requires db=full and your configuration is db=%s, it has been changed automatically..." % (self.options.db,))
                self.options.db = "full"


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
        KnuthConanFile.package_id(self)
        self.info.options.tools = "ANY"

    def build(self):
        cmake = self.cmake_basis()
        cmake.definitions["WITH_CONSENSUS"] = option_on_off(self.options.consensus)
        cmake.definitions["WITH_KEOKEN"] = option_on_off(self.is_keoken)
        # cmake.definitions["WITH_MINING"] = option_on_off(self.options.mining)
        cmake.definitions["WITH_MEMPOOL"] = option_on_off(self.options.mempool)
        cmake.definitions["DB_READONLY_MODE"] = option_on_off(self.options.db_readonly)
        cmake.definitions["LOG_LIBRARY"] = self.options.log
        cmake.definitions["USE_LIBMDBX"] = option_on_off(self.options.use_libmdbx)

        cmake.configure(source_dir=self.source_folder)
        if not self.options.cmake_export_compile_commands:
            cmake.build()
            if self.options.tests:
                cmake.test()

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.ipp", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libs = ["kth-blockchain"]
