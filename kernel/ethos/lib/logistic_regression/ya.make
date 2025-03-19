LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/containers/dense_hash
    kernel/ethos/lib/data
    kernel/ethos/lib/features_selector
    kernel/ethos/lib/linear_classifier_options
    kernel/ethos/lib/linear_model
    kernel/ethos/lib/metrics
    kernel/ethos/lib/out_of_fold
    kernel/ethos/lib/util
)

SRCS(
    logistic_regression.h
    logistic_regression.cpp
)

END()
