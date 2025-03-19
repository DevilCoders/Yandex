LIBRARY()

OWNER(
    olegator
    g:search-pers
)

SRCS(
    user_history.cpp
)

PEERDIR(
    kernel/user_history/proto
    library/cpp/binsaver
)

END()
