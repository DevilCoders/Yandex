OWNER(
    g:base
    g:wizard
    leo
    onpopov
)

UNITTEST()

PEERDIR(
    kernel/reqerror
    ADDINCL kernel/qtree/request
)

SRCDIR(kernel/qtree/request)

SRCS(
    reqattrlist_ut.cpp
)

END()
