Approximate nearest neighbor search via HNSW built on clustered vectors
=====================================================

**Introduction.**

HNSW (library/cpp/hnsw) offers a fast and precise approximate nearest neighbor search. However, HNSW index
consumes O(num neighbors) memory per vector in the structure. For 50-byte DSSM vectors it is 128 bytes per vector (32 neighbors)
to get a reasonable quality, which is a lot.

This library implements clustering heuristic to reduce memory usage by sacrificing performance.

**Building the index**

1. Apply clustering (e.g. library/cpp/kmeans_hnsw) to group 'similar' vectors into clusters of size C ~ 32
2. Possibly apply the overlapping clusters heuristic (see below) to make clusters overlapping to increase the search quality.
The average cluster size will grow from C to C*alpha (alpha depends on the vector distribution; for DSSM vectors I used in testing, alpha is ~1.5).
3. Build HNSW index on cluster centroids, use ~C neighbors per centroid.

**Processing query**

1. Query HNSW index to find the top K clusters nearest to the query
2. Consider all vectors from the top K clusters (~CK*alpha vectors)

The storage size is therefore reduced to ~N neighbor ids (~C neighbors for ~N/C clusters in the biggest layer) for HNSW index and N*alpha vector ids for clustering data.
The total number of ids stored becomes (1+alpha) ~ 2.5 per vector, as opposed to 32 in HNSW.

The performance heavily depends on the desired quality and the vector distribution; experiments are required.
For DSSM vectors I used it is possible to get the quality of HNSW by performing 4x more distance queries.

**Overlapping clusters heuristic**

When the clustering is done, the idea is to try to add each vector into several nearest clusters.

The following heuristic is proposed to select which clusters to add vectors to:

When trying to add the vector to the cluster i (clusters are sorted by distance in ascending order; try to add each vector to the
nearest ~20 clusters),
skip that cluster if there is a cluster j<i such that the cluster j is closer to i than the vector.

It turns out that this heuristic adds the vector on average to alpha~1.5 clusters (including the initial cluster) for DDSM vectors.

**Usage**

Index building and search interface resembles those of library/cpp/hnsw. Refer to clustered_hnsw_ut.cpp for a usage example.
