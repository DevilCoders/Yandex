LIBRARY()

OWNER(g:antirobot)

SRCS(
    rcu.cpp
    rcu_accessor.cpp
)

PEERDIR(
    library/cpp/threading/future
)

END()
