LIBRARY()

OWNER(g:cs_dev)

SRCS(
    datetime.h
    datetime.cpp
)

PEERDIR(
    util/draft
    kernel/common_server/util/math
)

END()

RECURSE_FOR_TESTS(
    ut
)
