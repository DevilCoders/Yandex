LIBRARY()

OWNER(
    g:saas
)

SRCS(
    GLOBAL hash.cpp
)

PEERDIR(
    library/cpp/logger/global
    kernel/multipart_archive/common
    kernel/multipart_archive/archive_impl
    kernel/multipart_archive
)

END()
