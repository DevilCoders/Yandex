GTEST()

OWNER(g:yt)

SRCS(
    recursive_spin_lock_ut.cpp
    spin_wait_ut.cpp
)

PEERDIR(
    library/cpp/yt/assert
    library/cpp/yt/threading
    library/cpp/testing/gtest
)

END()
