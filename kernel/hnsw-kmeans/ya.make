LIBRARY()

OWNER(g:base)

SRCS(kmeans.cpp)

PEERDIR(
    kernel/hnsw-kmeans/protos
    kernel/hnsw-kmeans/lib
    kernel/dssm_applier/decompression
    library/cpp/getopt
    library/cpp/getopt/small
    library/cpp/hnsw/index
    library/cpp/hnsw/index_builder
    mapreduce/yt/client
    robot/jupiter/library/tables
    robot/library/yt/static
)

END()

