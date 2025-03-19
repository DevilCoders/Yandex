LIBRARY()

OWNER(
    g:saas
)

SRCS(
    hash.cpp
)

PEERDIR(
    kernel/multipart_archive/protos
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/json
)

END()
