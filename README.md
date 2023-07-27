# MPC verifier

Please refer to [BGW test readme](test/bgw/README.md).

## Clone & Dependency

Since we rely on a third-party library `immer`, after cloning this repository, you could use the following commands at the root path of this repository to fetch `immer`.
```bash
git submodule init
git submodule update
```

Then, please copy `immer/immer` somewhere in your *include* path, for example,
```bash
sudo cp -r immer/immer/ /usr/local/include
```

## Build

调试版：

```sh
mkdir -p build && cd build
cmake .. -B Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug --target test_add # test_add 可以换成任何想测试的 cpp 文件，比如 test_bin2arith
```

正式版：

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

WARNING: cannot be correctly proved yet.

## Roadmap

- [ ] Xingyu feels that if it is possible to execute the graph, its design and implementation will be much better, and omitted details (e.g. the value range of this node) could be discovered and perfected.
- [ ] Xingyu feels that more documentation will make our lives better.
- [ ] Xingyu feels that we may need a more robust logging system. Maybe some third party logging library could help us, for example, [spdlog](https://github.com/gabime/spdlog/).
