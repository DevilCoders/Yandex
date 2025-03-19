LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    library/cpp/containers/dense_hash
    kernel/ethos/lib/data
    kernel/ethos/lib/linear_classifier_options
    kernel/ethos/lib/out_of_fold
)

SRCS(
    binary_model.h
    binary_model.cpp

    multi_model.h
    multi_model.cpp
)

END()
