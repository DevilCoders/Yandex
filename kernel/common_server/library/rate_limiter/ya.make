LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/async_impl
)

SRCS(
    rate_limiter.cpp
)

END()
