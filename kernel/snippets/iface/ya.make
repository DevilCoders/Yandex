LIBRARY()

OWNER(g:snippets)

SRCS(
    passagereply.cpp
    passagereplyfiller.cpp
)

PEERDIR(
    kernel/matrixnet
    kernel/qtree/richrequest
    kernel/search_daemon_iface
    kernel/snippets/hits
    kernel/snippets/idl
    kernel/snippets/iface/archive
    kernel/snippets/strhl
    kernel/snippets/urlcut
    library/cpp/string_utils/base64
    search/idl
)

END()
