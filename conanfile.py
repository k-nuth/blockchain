# Copyright (c) 2016-2020 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from conans import CMake
from ci_utils import option_on_off, march_conan_manip, pass_march_to_compiler
from ci_utils import KnuthConanFile

class KnuthBlockchainConan(KnuthConanFile):
    name = "blockchain"
    # version = get_version()
    license = "http://www.boost.org/users/license.html"
    url = "https://github.com/k-nuth/blockchain/blob/conan-build/conanfile.py"
    description = "Bitprim Blockchain Library"
    settings = "os", "compiler", "build_type", "arch"

    # if Version(conan_version) < Version(get_conan_req_version()):
    #     raise Exception ("Conan version should be greater or equal than %s. Detected: %s." % (get_conan_req_version(), conan_version))

    options = {"shared": [True, False],
               "fPIC": [True, False],
               "with_consensus": [True, False],
               "with_tests": [True, False],
               "with_tools": [True, False],
               "currency": ['BCH', 'BTC', 'LTC'],
               "microarchitecture": "ANY", #["x86_64", "haswell", "ivybridge", "sandybridge", "bulldozer", ...]
               "fix_march": [True, False],
               "verbose": [True, False],
               "keoken": [True, False],
            #    "mining": [True, False],
               "mempool": [True, False],
               "use_domain": [True, False],
               "db": ['legacy', 'legacy_full', 'pruned', 'default', 'full'],
               "cxxflags": "ANY",
               "cflags": "ANY",
               "glibcxx_supports_cxx11_abi": "ANY",
    }
    # "with_remote_database": [True, False],

    default_options = "shared=False", \
        "fPIC=True", \
        "with_consensus=True", \
        "with_tests=False", \
        "with_tools=False", \
        "currency=BCH", \
        "microarchitecture=_DUMMY_",  \
        "fix_march=False", \
        "verbose=False", \
        "keoken=False", \
        "mempool=False", \
        "use_domain=True", \
        "db=default", \
        "cxxflags=_DUMMY_", \
        "cflags=_DUMMY_", \
        "glibcxx_supports_cxx11_abi=_DUMMY_"

        # "mining=False", \

    # "with_remote_database=False"

    generators = "cmake"
    exports = "conan_*", "ci_utils/*"
    exports_sources = "src/*", "CMakeLists.txt", "cmake/*", "bitprim-blockchainConfig.cmake.in", "knuthbuildinfo.cmake", "include/*", "test/*", "test_new/*", "tools/*"
    package_files = "build/lbitprim-blockchain.a"
    build_policy = "missing"

    @property
    def is_keoken(self):
        return self.options.currency == "BCH" and self.options.get_safe("keoken")

    def requirements(self):
        if self.options.use_domain:
            self.requires("boost/1.69.0@kth/stable")
        else:
            self.requires("boost/1.66.0@kth/stable")

        self.requires("kth-database/0.X@%s/%s" % (self.user, self.channel))

        if self.options.with_consensus:
            self.requires.add("bitprim-consensus/0.X@%s/%s" % (self.user, self.channel))

    def config_options(self):
        if self.settings.arch != "x86_64":
            self.output.info("microarchitecture is disabled for architectures other than x86_64, your architecture: %s" % (self.settings.arch,))
            self.options.remove("microarchitecture")
            self.options.remove("fix_march")

        if self.settings.compiler == "Visual Studio":
            self.options.remove("fPIC")
            if self.options.shared and self.msvc_mt_build:
                self.options.remove("shared")

        if self.options.keoken and self.options.currency != "BCH":
            self.output.warning("Keoken is only enabled for BCH, for the moment. Removing Keoken support")
            self.options.remove("keoken")


    def configure(self):
        KnuthConanFile.configure(self)

        if self.settings.arch == "x86_64" and self.options.microarchitecture == "_DUMMY_":
            del self.options.fix_march
            # self.options.remove("fix_march")
            # raise Exception ("fix_march option is for using together with microarchitecture option.")

        if self.settings.arch == "x86_64":
            march_conan_manip(self)
            self.options["*"].microarchitecture = self.options.microarchitecture

        if self.options.keoken and self.options.currency != "BCH":
            self.output.warn("For the moment Keoken is only enabled for BCH. Building without Keoken support...")
            del self.options.keoken
        else:
            self.options["*"].keoken = self.options.keoken

        if self.is_keoken:
            if self.options.db == "pruned" or self.options.db == "default":
                self.output.warn("Keoken mode requires db=full and your configuration is db=%s, it has been changed automatically..." % (self.options.db,))
                self.options.db = "full"


        self.options["*"].db = self.options.db

        self.options["*"].use_domain = self.options.use_domain
        # self.options["*"].mining = self.options.mining
        # self.options["*"].mempool = self.options.mempool  
        self.options["*"].currency = self.options.currency
        self.output.info("Compiling for currency: %s" % (self.options.currency,))
        # self.output.info("Compiling with mining optimizations: %s" % (self.options.mining,))
        self.output.info("Compiling with mempool: %s" % (self.options.mempool,))
        self.output.info("Compiling for DB: %s" % (self.options.db,))

    def package_id(self):
        KnuthConanFile.package_id(self)

        self.info.options.with_tests = "ANY"
        self.info.options.with_tools = "ANY"
        self.info.options.verbose = "ANY"
        self.info.options.fix_march = "ANY"
        self.info.options.cxxflags = "ANY"
        self.info.options.cflags = "ANY"

        # #For Bitprim Packages libstdc++ and libstdc++11 are the same
        # if self.settings.compiler == "gcc" or self.settings.compiler == "clang":
        #     if str(self.settings.compiler.libcxx) == "libstdc++" or str(self.settings.compiler.libcxx) == "libstdc++11":
        #         self.info.settings.compiler.libcxx = "ANY"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["USE_CONAN"] = option_on_off(True)
        cmake.definitions["NO_CONAN_AT_ALL"] = option_on_off(False)
        cmake.verbose = self.options.verbose
        cmake.definitions["ENABLE_SHARED"] = option_on_off(self.is_shared)
        cmake.definitions["ENABLE_POSITION_INDEPENDENT_CODE"] = option_on_off(self.fPIC_enabled)
        cmake.definitions["WITH_CONSENSUS"] = option_on_off(self.options.with_consensus)
        # cmake.definitions["WITH_LITECOIN"] = option_on_off(self.options.with_litecoin)
        # cmake.definitions["WITH_REMOTE_DATABASE"] = option_on_off(self.options.with_remote_database)
        cmake.definitions["WITH_TESTS"] = option_on_off(self.options.with_tests)
        cmake.definitions["WITH_TESTS_NEW"] = option_on_off(self.options.with_tests)

        cmake.definitions["WITH_TOOLS"] = option_on_off(self.options.with_tools)
        cmake.definitions["WITH_KEOKEN"] = option_on_off(self.is_keoken)
        cmake.definitions["CURRENCY"] = self.options.currency
        # cmake.definitions["WITH_MINING"] = option_on_off(self.options.mining)
        cmake.definitions["WITH_MEMPOOL"] = option_on_off(self.options.mempool)
        cmake.definitions["USE_DOMAIN"] = option_on_off(self.options.use_domain)


        if self.options.db == "legacy":
            cmake.definitions["DB_TRANSACTION_UNCONFIRMED"] = option_on_off(False)
            cmake.definitions["DB_SPENDS"] = option_on_off(False)
            cmake.definitions["DB_HISTORY"] = option_on_off(False)
            cmake.definitions["DB_STEALTH"] = option_on_off(False)
            cmake.definitions["DB_UNSPENT_LIBBITCOIN"] = option_on_off(True)
            cmake.definitions["DB_LEGACY"] = option_on_off(True)
            cmake.definitions["DB_NEW"] = option_on_off(False)
            cmake.definitions["DB_NEW_BLOCKS"] = option_on_off(False)
            cmake.definitions["DB_NEW_FULL"] = option_on_off(False)
        elif self.options.db == "legacy_full":
            cmake.definitions["DB_TRANSACTION_UNCONFIRMED"] = option_on_off(True)
            cmake.definitions["DB_SPENDS"] = option_on_off(True)
            cmake.definitions["DB_HISTORY"] = option_on_off(True)
            cmake.definitions["DB_STEALTH"] = option_on_off(True)
            cmake.definitions["DB_UNSPENT_LIBBITCOIN"] = option_on_off(True)
            cmake.definitions["DB_LEGACY"] = option_on_off(True)
            cmake.definitions["DB_NEW"] = option_on_off(False)
            cmake.definitions["DB_NEW_BLOCKS"] = option_on_off(False)
            cmake.definitions["DB_NEW_FULL"] = option_on_off(False)
        elif self.options.db == "pruned":
            cmake.definitions["DB_TRANSACTION_UNCONFIRMED"] = option_on_off(False)
            cmake.definitions["DB_SPENDS"] = option_on_off(False)
            cmake.definitions["DB_HISTORY"] = option_on_off(False)
            cmake.definitions["DB_STEALTH"] = option_on_off(False)
            cmake.definitions["DB_UNSPENT_LIBBITCOIN"] = option_on_off(False)
            cmake.definitions["DB_LEGACY"] = option_on_off(False)
            cmake.definitions["DB_NEW"] = option_on_off(True)
            cmake.definitions["DB_NEW_BLOCKS"] = option_on_off(False)
            cmake.definitions["DB_NEW_FULL"] = option_on_off(False)
        elif self.options.db == "default":
            cmake.definitions["DB_TRANSACTION_UNCONFIRMED"] = option_on_off(False)
            cmake.definitions["DB_SPENDS"] = option_on_off(False)
            cmake.definitions["DB_HISTORY"] = option_on_off(False)
            cmake.definitions["DB_STEALTH"] = option_on_off(False)
            cmake.definitions["DB_UNSPENT_LIBBITCOIN"] = option_on_off(False)
            cmake.definitions["DB_LEGACY"] = option_on_off(False)
            cmake.definitions["DB_NEW"] = option_on_off(True)
            cmake.definitions["DB_NEW_BLOCKS"] = option_on_off(True)
            cmake.definitions["DB_NEW_FULL"] = option_on_off(False)
        elif self.options.db == "full":
            cmake.definitions["DB_TRANSACTION_UNCONFIRMED"] = option_on_off(False)
            cmake.definitions["DB_SPENDS"] = option_on_off(False)
            cmake.definitions["DB_HISTORY"] = option_on_off(False)
            cmake.definitions["DB_STEALTH"] = option_on_off(False)
            cmake.definitions["DB_UNSPENT_LIBBITCOIN"] = option_on_off(False)
            cmake.definitions["DB_LEGACY"] = option_on_off(False)
            cmake.definitions["DB_NEW"] = option_on_off(True)
            cmake.definitions["DB_NEW_BLOCKS"] = option_on_off(False)
            cmake.definitions["DB_NEW_FULL"] = option_on_off(True)

        if self.settings.compiler != "Visual Studio":
            cmake.definitions["CONAN_CXX_FLAGS"] = cmake.definitions.get("CONAN_CXX_FLAGS", "") + " -Wno-deprecated-declarations"

        if self.settings.compiler == "Visual Studio":
            cmake.definitions["CONAN_CXX_FLAGS"] = cmake.definitions.get("CONAN_CXX_FLAGS", "") + " /DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE"

        if self.options.cxxflags != "_DUMMY_":
            cmake.definitions["CONAN_CXX_FLAGS"] = cmake.definitions.get("CONAN_CXX_FLAGS", "") + " " + str(self.options.cxxflags)
        if self.options.cflags != "_DUMMY_":
            cmake.definitions["CONAN_C_FLAGS"] = cmake.definitions.get("CONAN_C_FLAGS", "") + " " + str(self.options.cflags)


        cmake.definitions["MICROARCHITECTURE"] = self.options.microarchitecture
        cmake.definitions["KTH_PROJECT_VERSION"] = self.version

        if self.settings.compiler == "gcc":
            if float(str(self.settings.compiler.version)) >= 5:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(False)
            else:
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(True)
        elif self.settings.compiler == "clang":
            if str(self.settings.compiler.libcxx) == "libstdc++" or str(self.settings.compiler.libcxx) == "libstdc++11":
                cmake.definitions["NOT_USE_CPP11_ABI"] = option_on_off(False)

        pass_march_to_compiler(self, cmake)

        cmake.configure(source_dir=self.source_folder)
        cmake.build()

        if self.options.with_tests:
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
        self.cpp_info.libs = ["bitprim-blockchain"]
