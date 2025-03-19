LIBRARY()

OWNER(
    alex-sh
)

PEERDIR(
    library/cpp/containers/dense_hash

    library/cpp/getopt/small

    library/cpp/linear_regression

    kernel/ethos/lib/util
    kernel/ethos/lib/data
)

SRCS(
    pca.h
    pca.cpp
)

END()
