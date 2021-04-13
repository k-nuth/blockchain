#!/bin/bash

set -x

# sudo apt-get update

# apt-get install clang-tidy-6.0
#PATH=/usr/lib/llvm-6.0/bin/:$PATH

# sudo apt-get install clang-tidy-7
# PATH=/usr/lib/llvm-7/bin/:$PATH


wget http://releases.llvm.org/7.0.0/clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
# tar -xJvf clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar -xJf clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz
ls /home/conan/clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04/bin
PATH=/home/conan/clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04/bin/:$PATH

sudo find / -name "clang-tidy"
clang-tidy --version

conan user
conan --version

conan remote add kth_temp https://knuth.jfrog.io/artifactory/api/conan/knuth

cd /home/conan/project

echo $CONAN_CHANNEL
eval $(python ci_utils/set_envvars_and_files.py)
echo $CONAN_CHANNEL

# git clone --depth=50 --branch=dev https://github.com/k-nuth/domain.git kth/domain
# cd kth/domain
# git submodule update --init --recursive

mkdir build
cd build
conan install ..
# cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DUSE_CONAN=ON -DENABLE_TESTS=OFF -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DJUST_KTH_SOURCES=OFF -DWITH_KEOKEN=ON
# python ../ci_utils/run-clang-tidy.py -checks='*,-android-*,-llvm-header-guard,-fuchsia*,-google-runtime-references,-cert-err58-cpp,-cppcoreguidelines-special-member-functions,-modernize-pass-by-value,-cert-dcl21-cpp,-google-explicit-constructor,-hicpp-explicit-conversions,-hicpp-member-init,-hicpp-signed-bitwise,-google-build-using-namespace,-hicpp-special-member-functions,-cppcoreguidelines-pro-type-member-init,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-hicpp-no-array-decay' -warnings-as-errors='*'  -header-filter=/home/conan/project/include/

cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DUSE_CONAN=ON  -DENABLE_TESTS=OFF -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DJUST_KTH_SOURCES=ON -DWITH_KEOKEN=ON
python ../ci_utils/run-clang-tidy.py -checks='*,-android-*,-modernize-use-using,-hicpp-deprecated-headers,-modernize-deprecated-headers,-cppcoreguidelines-pro-type-vararg,-cppcoreguidelines-owning-memory,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-hicpp-vararg,-hicpp-no-malloc,-fuchsia-default-arguments,-google-runtime-references,-cppcoreguidelines-no-malloc,-hicpp-no-malloc,-llvm-header-guard,-android-cloexec-fopen,-cppcoreguidelines-pro-type-union-access,-cppcoreguidelines-pro-type-const-cast,-hicpp-braces-around-statements,-hicpp-special-member-functions,-readability-braces-around-statements,-coreguidelines-special-member-function,-cppcoreguidelines-special-member-functions,-clang-diagnostic-static-inline-explicit-instantiation,-fuchsia-overloaded-operator,-cppcoreguidelines-pro-type-member-init,-hicpp-member-init' -warnings-as-errors='*' -header-filter=/home/conan/project/include/kth
