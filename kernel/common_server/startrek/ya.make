LIBRARY()

OWNER(g:cs_dev)

SRCS(
    issue_requests.cpp
    comment_requests.cpp
    objects.cpp
)

PEERDIR(
    kernel/daemon/config
    kernel/common_server/abstract
    kernel/common_server/library/json
    kernel/common_server/library/tvm_services
)

END()
