LIBRARY()

OWNER(alex-sh)

SRCS(
    agglomerative_clustering.cpp
    agglomerative_clustering.h
)

PEERDIR(
    contrib/libs/sparsehash
    kernel/cluster/lib/cluster_metrics
    library/cpp/containers/dense_hash
    library/cpp/containers/intrusive_rb_tree
    library/cpp/deprecated/atomic
)

END()
