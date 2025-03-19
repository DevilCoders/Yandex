LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    kernel/ethos/lib/linear_classifier_options
    library/cpp/linear_regression
)

SRCS(
    dataset.h
    linear_model.h
    vs_happy.cpp
)

END()
