# Parallelization of single-threaded algorithms

Code for the individual classification for the [VI Spanish Parallel Programming Contest (2016)](http://luna.inf.um.es/2016/results.php?lang=en).
Code results from the competition by team can be seen in [records' table](http://luna.inf.um.es/2016/records.php).

## Problem description
The algorithm solves the problem of backpacks with affinity, where the content of a backpack cannot exceed the maximum weight supported by the backpack and where the benefit of a backpack is calculated according to an affinity. The afinity is a benefit obtained if two objects are in the same bag. The number of backpacks and items can be any.

## Results on CESGA cluster (2016)

| Implementation | Speedup |
|----------|:-------------:|
| OMP | 31.08 |
| MPI | 8.37 |
| MPI+OMP | 10.55 |
| CUDA |  1.33 |
