LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/cgiparam
    kernel/common_server/auth/common
)

SRCS(
    GLOBAL apikey.cpp
)

END()
