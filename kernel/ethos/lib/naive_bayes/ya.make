LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/containers/dense_hash
    library/cpp/linear_regression
    kernel/ethos/lib/data
    kernel/ethos/lib/features_selector
    kernel/ethos/lib/linear_classifier_options
    kernel/ethos/lib/linear_model
    kernel/ethos/lib/metrics
    kernel/ethos/lib/util
)

SRCS(
    naive_bayes.h
    naive_bayes.cpp
)

END()
