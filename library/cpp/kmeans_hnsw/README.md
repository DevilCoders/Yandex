K-Means clustering algorithm implementation with HNSW for approximate nearest neighbor search
=====================================================

The library implements K-Means clustering algorithm, which is

1. Initialize cluster centers at random
2. Repeat the following steps given number of iterations:
3. Put each vector to the cluster with the nearest cluster center to that vector
4. Output current clustering if sufficient number of iterations is done
5. For each cluster, recalculate cluster center as a centroid of the vectors in the cluster

Step 3 is done approximately by HNSW index (library/cpp/hnsw).

The library accepts and outputs vectors through TItemStorage interface which is introduced in library/cpp/hnsw (refer to
library/cpp/hnsw/index_builder/dense_vector_storage.h and library/cpp/hnsw/ut/main.cpp).

Additionally, TCentroidsFactory interface is introduced to encapsulate calculation of centroids of sets of vectors. The
library provides an implementation of this factory for NHnsw::TDenseVectorStorage in dense_vector_centroids_factory.h

Refer to ut/main.cpp for a self-explanatory usage example.
