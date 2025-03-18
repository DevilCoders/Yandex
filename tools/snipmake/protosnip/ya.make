LIBRARY()

OWNER(
    divankov
    g:snippets
)

SRCS(
    patch.cpp
)

PEERDIR(
    kernel/search_daemon_iface
    kernel/snippets/iface
    library/cpp/scheme
    search/idl
)

END()
