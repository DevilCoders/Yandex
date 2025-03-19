LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/signature/abstract
    library/cpp/digest/md5
)

SRCS(
    md5signature.cpp
)

END()
