LIBRARY()

OWNER(g:saas)

SRCS(
    archive_info.cpp
)

PEERDIR(
    library/cpp/json
    kernel/multipart_archive/abstract
)

END()
