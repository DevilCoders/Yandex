OWNER(g:base)

LIBRARY()

PEERDIR(
    kernel/hitinfo
    kernel/factor_slices
    kernel/search_daemon_iface
    kernel/search_types
    library/cpp/wordpos
)

SRCS(
    relev.h
    relev.cpp
    snippets_hits.h
)

END()
