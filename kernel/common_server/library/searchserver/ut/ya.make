UNITTEST_FOR(kernel/common_server/library/searchserver)

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/searchserver
    kernel/common_server/library/searchserver/simple/context
    kernel/common_server/obfuscator/obfuscators
    search/request/data
)

SRCS(
    replier_ut.cpp
)

END()
