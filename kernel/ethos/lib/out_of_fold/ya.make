LIBRARY()

OWNER(
    alex-sh
    druxa
)

PEERDIR(
    kernel/ethos/lib/data
    kernel/ethos/lib/linear_classifier_options
    kernel/ethos/lib/util
)

SRCS(
    out_of_fold.h
    out_of_fold.cpp
)

END()
