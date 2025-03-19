LIBRARY()

OWNER(g:ugc)

PEERDIR(
    kernel/ugc/security/lib
    kernel/ugc/aggregation/proto
    library/cpp/scheme
    library/cpp/json
)

SRCS(
    feedback.cpp
    feedback_v2.cpp
    utils.cpp
)

END()
