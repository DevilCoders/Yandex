UNITTEST()

OWNER(smikler)

PEERDIR(
    ADDINCL kernel/urlnorm
)

SRCDIR(kernel/urlnorm)

SRCS(
    normalize_ut.cpp
    urlhashval_ut.cpp
)

END()
