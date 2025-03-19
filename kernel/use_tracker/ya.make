LIBRARY()

OWNER(ssmike)

SRCS(
    tracker.cpp
)

PEERDIR(
    library/cpp/threading/future
    library/cpp/threading/hot_swap
    library/cpp/deprecated/atomic
)

END()
