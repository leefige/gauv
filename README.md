# MPC verifier

## Dependencies

Since we rely on third-party libraries (immer and spdlog), after cloning this repository, you could use the following commands at the root path of this repository to fetch the third-party libraries.
```bash
git submodule init
git submodule update
```

Then, please copy `immer/immer/` and `spdlog/include/spdlog/` somewhere in your *include* path, for example,
```bash
sudo cp -r immer/immer/ /usr/local/include
sudo cp -r spdlog/include/spdlog/ /usr/local/include
```

## Build

We have two versions: debug version and release version.
The debug version prints more logs but runs slower, while the release version runs faster but prints less logs.

Debug version:

```sh
mkdir -p build && cd build
cmake .. -B Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug --target test_add # "test_add" could be any `.cpp`-file to test, like, "test_bin2arith"
```

Release version:

```sh
mkdir -p build && cd build
cmake .. -B Release -DCMAKE_BUILD_TYPE=Release
cmake --build Release
```

## BGW

The main constraint for our program is the size of the BGW graph instead of the running time.

1. `test/bgw/test_add 8 10 21 100`
2. `test/bgw/test_mul 8 10 21 30` (potential OOM)
3. Loop over I from 1 to 5 for `test/bgw/test_mul I 5 11 20`
4. Try different arguments for `test/bgw/test_scalable`, e.g.
   1. `test/bgw/test_scalable 20 20 115 1` (3,078,550 nodes, 2,949,635 edges)
   2. `test/bgw/test_scalable 20 20 50 5` (1,374,800 nodes, 1,247,550 edges)

## Conversion

`test/conversion/test_bin2arith 2 3 4`

## Logging

We use a library spdlog for logging. There are six logging level (from small to large): `trace`, `debug`, `info`, `warn`, `error`, `critical`. The debug version records the logs of all levels, while the release version records the logs larger than `debug` (i.e., `info`, `warn`, `error` and `critical`).

## Roadmap

- [ ] Xingyu feels that if it is possible to execute the graph, its design and implementation will be much better, and omitted details (e.g. the value range of this node) could be discovered and perfected.
- [ ] Xingyu feels that more documentation will make our lives better.
