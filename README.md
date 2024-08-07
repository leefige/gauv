GAuV: A Graph-Based Automated Verification Framework for Perfect Semi-Honest Security of Multiparty Computation Protocols
======

[![DOI](https://zenodo.org/badge/728296326.svg)](https://zenodo.org/doi/10.5281/zenodo.10277757)

GAuV is a automated framework for verifying the perfect security of multiparty computation (MPC) protocol against semi-honest adversaries.
GAuV accepts a MPC protocol representation and produces YES/UNKNOWN in a push-button style (without any other human intervention).

## Dependencies

We rely on two third party projects: [immer](https://github.com/immerjs/immer) and [spdlog](https://github.com/gabime/spdlog), which you can see in `external/`.

We rely on [OpenMP](https://www.openmp.org/) to leverage prallel for speedup. Usually, the developing library of OpenMP can be installed by a package manager, e.g., `apt install libomp-dev`.

## Build

We have two versions: debug version and release version.
The debug version prints more logs but runs slower, while the release version runs faster but prints less logs.

Debug version:

```sh
mkdir -p build && cd build
cmake .. -B Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug
```

Release version:

```sh
mkdir -p build && cd build
cmake .. -B Release -DCMAKE_BUILD_TYPE=Release
cmake --build Release
```

## How to run?

The protocols are described in `example/`.
At present, we always require some arguments like the number of parties.

### BGW

We can try to verify a BGW protocol as follows, where the argument `I` means all possible valid corrupted party sets are considered, `2` means the threshold of the number of corrupted parties, `5` means the number of parties, and `10` means the size of circuits.
Given the size of the circuits, a random circuit of certain size will be generated, and accordingly a BGW protocol.

```sh
example/bgw/test_scalable I 2 5 10
```

### B2A

We can try to verify a B2A (binary to arithmetic) protocol, in which the underlying secret sharing scheme is Shamir secret sharing as BGW protocols, as follows, where `I` means all possible valid corrupted party sets are considered, `1` means the threshold of the number of corrupted parties, and `3` means the number of parties.

```sh
example/conversion/test_bin2arith I 1 3
```

### Reproduce the Evaluation

To reproduce the evaluation in our S&P'24 paper, run the shell scripts, `script-bin2arith.sh` and `script-scalable.sh`, in `sp24-scripts/`.

## Citation

If you find our work helpful, feel free to cite our [paper](https://eprint.iacr.org/2024/1078).

```bibtex
@INPROCEEDINGS {gauv2024,
  author = {Xingyu Xie and Yifei Li and Wei Zhang and Tuowei Wang and Shizhen Xu and Jun Zhu and Yifan Song},
  booktitle = {2024 IEEE Symposium on Security and Privacy (SP)},
  title = {GAuV: A Graph-Based Automated Verification Framework for Perfect Semi-Honest Security of Multiparty Computation Protocols},
  year = {2024}
}
```

## Note

This project is developed only for research purpose. If you have any questions, please contact us!

An unfinished part of this project, which is located in `src/mpclib`, is an 'interpreter' for running the protocol, if anyone is interested in developing this further, please go for it!
