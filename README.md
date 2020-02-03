# Bitprim Blockchain <a target="_blank" href="https://gitter.im/bitprim/Lobby">![Gitter Chat][badge.Gitter]</a>

*Bitcoin blockchain library*

| **master(linux/osx)** | **dev(linux/osx)**   | **master(windows)**   | **dev(windows)** |
|:------:|:-:|:-:|:-:|
| [![Build Status](https://travis-ci.org/k-nuth/blockchain.svg)](https://travis-ci.org/k-nuth/blockchain)       | [![Build StatusB](https://travis-ci.org/k-nuth/blockchain.svg?branch=dev)](https://travis-ci.org/k-nuth/blockchain?branch=dev)  | [![Appveyor Status](https://ci.appveyor.com/api/projects/status/github/k-nuth/blockchain?svg=true)](https://ci.appveyor.com/project/k-nuth/blockchain)  | [![Appveyor StatusB](https://ci.appveyor.com/api/projects/status/github/k-nuth/blockchain?branch=dev&svg=true)](https://ci.appveyor.com/project/k-nuth/blockchain?branch=dev)  |

Make sure you have installed [kth-domain](https://github.com/k-nuth/core), [kth-database](https://github.com/k-nuth/database) and [bitprim-consensus](https://github.com/k-nuth/consensus) (optional) beforehand according to their respective build instructions.

```
$ git clone https://github.com/k-nuth/blockchain.git
$ cd bitprim-blockchain
$ mkdir build
$ cd build
$ cmake .. -DWITH_TESTS=OFF -DWITH_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-std=c++14"
$ make -j2 
$ sudo make install
```

bitprim-blockchain is now installed in `/usr/local/`.

## Configure Options

The default configuration requires `bitprim-consensus`. This ensures consensus parity with the Satoshi client. To eliminate the `bitprim-consensus` dependency use the `--without-consensus` option. This results in use of `kth-domain` consensus checks.

[badge.Gitter]: https://img.shields.io/badge/gitter-join%20chat-blue.svg
