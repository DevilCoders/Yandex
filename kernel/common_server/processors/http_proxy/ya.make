LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/tvm_services/abstract/request
    kernel/common_server/processors/common
    kernel/common_server/util/network
    library/cpp/digest/md5
    library/cpp/string_utils/url

)

SRCS(
    GLOBAL handler.cpp
)

END()
