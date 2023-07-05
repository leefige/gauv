# MPC verifier

Please refer to [BGW test readme](test/bgw/README.md).

## Build

```sh
cd test/bgw
make test_add test_mul
```

## Test cases

1. `test/bgw/test_add 8 10 21 100`
2. `test/bgw/test_mul 8 10 21 300`
3. Loop over I from 1 to 5 for `test/bgw/test_mul I 5 11 20`

**Following contents are out-dated!**

~~This project contains two major components:~~

~~1. An MPC DSL compiler (parser?) built with [ANTLR v4](https://github.com/antlr/antlr4);~~
~~2. A verifier for MPC protocols.~~

~~## Build for Linux~~

~~1. Download and put ANTLR jar under `lib/`;~~
~~2. Download, build and put the ANTLR C++ runtime under `runtime/Cpp`;~~
~~3. Build with `CMake`.~~
