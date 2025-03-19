UNITTEST_FOR(kernel/extended_mx_calcer/calcers)

OWNER(
    epar
)

SRCS(
    clickint_ut.cpp
    combinations_ut.cpp
    multifeature_softmax_ut.cpp
)

PEERDIR(
    library/cpp/scheme/ut_utils
)

END()
