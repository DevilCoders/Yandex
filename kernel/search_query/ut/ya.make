OWNER(
    ilnurkh
    timuratshin
    g:factordev
)

UNITTEST()

SIZE(MEDIUM)

PEERDIR(
    ADDINCL kernel/search_query
    library/cpp/resource
    ysite/yandex/reqanalysis
)

SRCDIR(kernel/search_query)

SRCS(
    search_query_ut.cpp
    cnorm_ut.cpp
)

FROM_SANDBOX(FILE 1205994907 OUT texts)

RESOURCE(
    texts /texts
)

END()
