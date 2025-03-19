LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/protobuf/json
    library/cpp/logger/global
    library/cpp/http/misc
    search/idl
    search/session/compression
    library/cpp/cgiparam
)

SRCS(
    simple.cpp
    abstract.cpp
)

END()
