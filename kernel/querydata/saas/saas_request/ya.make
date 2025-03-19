LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

PEERDIR(
    library/cpp/scheme
    kernel/querydata/cgi
    kernel/querydata/idl
    kernel/querydata/saas
    kernel/querydata/saas/idl
    kernel/saas_trie/idl
)

SRCS(
    trie.cpp
    saas_service_opts.cpp
)

END()
