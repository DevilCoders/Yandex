LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util
    taxi/logistic-dispatcher/api/search_client
    kernel/common_server/library/storage
    kernel/common_server/library/unistat
    kernel/common_server/util
)

SRCS(
    rty_kv_refresh.cpp
)

END()
