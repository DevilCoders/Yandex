LIBRARY()

OWNER(tobo)

PEERDIR(
    library/cpp/deprecated/split
)

SRCS(
    vowpal_wabbit_model.h
    vowpal_wabbit_model.cpp
    vowpal_wabbit_predictor.h
    vowpal_wabbit_predictor.cpp
    readable_repr.cpp
)

END()
