# DSCAN

DSCAN: Distributed Structural Graph Clustering for Billion-Edge Graphs.

## Compile

g++ should support c++11, e.g, g++ 4.8+, cmake should be 3.6+, allowing for modern cmake

```zsh
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER=/opt/intel/bin/icpc -DKNL=ON
make
```

## Usage

* converter(transform your edge list into `b_degree.bin`, `b_adj.bin`)

* DSCAN algorithm release(serial/parallel), the input files and output file are in `../dataset/toy_graph/` directory.
 `output` indicates outputs to the file.

