#!/bin/bash

function replace_versions {
    # echo $1 #project name
    # echo $2 #build number
    if [ ! -f versions.txt ]; then
        echo "$1: $2" >> versions.txt
    else
        while read p; do
            if [[ $p == *"$1:"* ]]; then
                echo "$1: $2" >> versions.txt.t
            else
                echo $p >> versions.txt.t
            fi
        done <versions.txt
        mv versions.txt{.t,}
    fi
}  

# --------------------------------------------------------------------------------------------------------------------
set -e
set -x

git config --global user.email "ci@bitprim.org"
git config --global user.name "Bitprim CI"

mkdir temp
cd temp


echo "Travis branch: ${TRAVIS_BRANCH}"
echo "Travis tag: ${TRAVIS_TAG}"

if [[ ${TRAVIS_BRANCH} == ${TRAVIS_TAG} ]]; then
    export BITPRIM_BRANCH=master
else
    export BITPRIM_BRANCH=${TRAVIS_BRANCH}
fi
echo "Bitprim branch: ${BITPRIM_BRANCH}"

# --------------------------------------------------------------------------------------------------------------------
# bitprim-node-exe
# --------------------------------------------------------------------------------------------------------------------
git clone https://github.com/bitprim/bitprim-node-exe.git

cd bitprim-node-exe
git checkout ${BITPRIM_BRANCH}

replace_versions bitprim-blockchain $BITPRIM_BUILD_NUMBER

cat versions.txt
git add . versions.txt
git commit --message "Travis bitprim-blockchain build: $BITPRIM_BUILD_NUMBER, $TRAVIS_BUILD_NUMBER" || true
git remote add origin-commit https://${GH_TOKEN}@github.com/bitprim/bitprim-node-exe.git > /dev/null 2>&1
git push --quiet --set-upstream origin-commit ${BITPRIM_BRANCH} || true

cd ..


# --------------------------------------------------------------------------------------------------------------------
# bitprim-node-cint
# --------------------------------------------------------------------------------------------------------------------
git clone https://github.com/bitprim/bitprim-node-cint.git

cd bitprim-node-cint
git checkout ${BITPRIM_BRANCH}

replace_versions bitprim-blockchain $BITPRIM_BUILD_NUMBER

cat versions.txt
git add . versions.txt
git commit --message "Travis bitprim-blockchain build: $BITPRIM_BUILD_NUMBER, $TRAVIS_BUILD_NUMBER" || true
git remote add origin-commit https://${GH_TOKEN}@github.com/bitprim/bitprim-node-cint.git > /dev/null 2>&1
git push --quiet --set-upstream origin-commit ${BITPRIM_BRANCH} || true

cd ..

# --------------------------------------------------------------------------------------------------------------------
