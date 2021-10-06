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

Shiokawa H., Takahashi T. (2020) DSCAN: Distributed Structural Graph Clustering for Billion-Edge Graphs. In: Hartmann S., Küng J., Kotsis G., Tjoa A.M., Khalil I. (eds) Database and Expert Systems Applications. DEXA 2020. Lecture Notes in Computer Science, vol 12391. Springer, Cham. https://doi.org/10.1007/978-3-030-59003-1_3
