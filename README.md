# MPC verifier

Please refer to [BGW test readme](test/bgw/README.md).

## Build

```sh
cd test/bgw
make test_add test_mul test_scalable
```

## Test cases

The main constraint for our program is the size of the BGW graph instead of the running time.

1. `test/bgw/test_add 8 10 21 100`
2. `test/bgw/test_mul 8 10 21 30` (potential OOM)
3. Loop over I from 1 to 5 for `test/bgw/test_mul I 5 11 20`
4. Try different arguments for `test/bgw/test_scalable`, e.g.
   1. `test/bgw/test_scalable 20 20 115 1` (3,078,550 nodes, 2,949,635 edges)
   2. `test/bgw/test_scalable 20 20 50 5` (1,374,800 nodes, 1,247,550 edges)

**Following contents are out-dated!**

~~This project contains two major components:~~

~~1. An MPC DSL compiler (parser?) built with [ANTLR v4](https://github.com/antlr/antlr4);~~
~~2. A verifier for MPC protocols.~~

~~## Build for Linux~~

~~1. Download and put ANTLR jar under `lib/`;~~
~~2. Download, build and put the ANTLR C++ runtime under `runtime/Cpp`;~~
~~3. Build with `CMake`.~~
