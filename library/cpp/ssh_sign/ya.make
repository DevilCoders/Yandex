LIBRARY()

OWNER(lkozhinov)

SRCS(
    policies.cpp
    sign.cpp
)

PEERDIR(
    library/cpp/ssh
)

END()

RECURSE_FOR_TESTS(ut)
