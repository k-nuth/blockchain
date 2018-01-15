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

import os
from conans import ConanFile, CMake

def option_on_off(option):
    return "ON" if option else "OFF"

class BitprimBlockchainConan(ConanFile):
    name = "bitprim-blockchain"
    version = "0.6"
    license = "http://www.boost.org/users/license.html"  #TODO(fernando): change to bitprim licence file
    url = "https://github.com/bitprim/bitprim-blockchain/blob/conan-build/conanfile.py"
    description = "Bitprim Blockchain Library"
    settings = "os", "compiler", "build_type", "arch"
    
    options = {"shared": [True, False],
               "fPIC": [True, False],
               "with_consensus": [True, False],
               "with_litecoin": [True, False]
    }

    # "with_remote_database": [True, False],
    # "with_tests": [True, False],
    # "with_tools": [True, False],
    # "not_use_cpp11_abi": [True, False]

    default_options = "shared=False", \
        "fPIC=True", \
        "with_consensus=True", \
        "with_litecoin=False"

    # "with_remote_database=False"
    # "with_tests=True", \
    # "with_tools=True", \
    # "not_use_cpp11_abi=False"

    with_tests = False
    with_tools = False


    generators = "cmake"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "bitprim-blockchainConfig.cmake.in", "include/*", "test/*", "tools/*"
    package_files = "build/lbitprim-blockchain.a"
    build_policy = "missing"

    requires = (("bitprim-conan-boost/1.64.0@bitprim/stable"),
                ("bitprim-database/0.6@bitprim/testing"))

    def requirements(self):
        if self.options.with_consensus:
            self.requires.add("bitprim-consensus/0.6@bitprim/testing")

    def build(self):
        cmake = CMake(self)
        
        cmake.definitions["USE_CONAN"] = option_on_off(True)
        cmake.definitions["NO_CONAN_AT_ALL"] = option_on_off(False)
        cmake.definitions["CMAKE_VERBOSE_MAKEFILE"] = option_on_off(False)
        cmake.definitions["ENABLE_SHARED"] = option_on_off(self.options.shared)
        cmake.definitions["ENABLE_POSITION_INDEPENDENT_CODE"] = option_on_off(self.options.fPIC)
        cmake.definitions["WITH_CONSENSUS"] = option_on_off(self.options.with_consensus)
        cmake.definitions["WITH_LITECOIN"] = option_on_off(self.options.with_litecoin)

        # cmake.definitions["WITH_REMOTE_DATABASE"] = option_on_off(self.options.with_remote_database)
        # cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(self.options.not_use_cpp11_abi)
        # cmake.definitions["WITH_TESTS"] = option_on_off(self.options.with_tests)
        # cmake.definitions["WITH_TOOLS"] = option_on_off(self.options.with_tools)
        cmake.definitions["WITH_TESTS"] = option_on_off(self.with_tests)
        cmake.definitions["WITH_TOOLS"] = option_on_off(self.with_tools)

        if self.settings.compiler == "gcc":
            if float(str(self.settings.compiler.version)) >= 5:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(False)
            else:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(True)

        cmake.definitions["BITPRIM_BUILD_NUMBER"] = os.getenv('BITPRIM_BUILD_NUMBER', '-')
        cmake.configure(source_dir=self.source_folder)
        cmake.build()

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
        self.cpp_info.libs = ["bitprim-blockchain"]
