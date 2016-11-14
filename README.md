[![Build Status](https://travis-ci.org/bitprim/bitprim-blockchain.svg?branch=master)](https://travis-ci.org/bitprim/bitprim-blockchain)

# Bitprim Blockchain

*Bitcoin blockchain library*

Make sure you have installed [bitprim-database](https://github.com/bitprim/bitprim-database) and [bitprim-consensus](https://github.com/bitprim/bitprim-consensus) (optional) beforehand according to their respective build instructions.

```sh
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

bitprim-blockchain is now installed in `/usr/local/`.

## Configure Options

The default configuration requires `bitprim-consensus`. This ensures consensus parity with the Satoshi client. To eliminate the `bitprim-consensus` dependency use the `--without-consensus` option. This results in use of `bitprim-core` consensus checks.
