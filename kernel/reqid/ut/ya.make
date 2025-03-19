UNITTEST()

OWNER(
    mvel
    g:base
    g:middle
)

PEERDIR(
    ADDINCL kernel/reqid
    library/cpp/scheme
)

SRCDIR(kernel/reqid)

SRCS(
    reqid_ut.cpp
)

END()
