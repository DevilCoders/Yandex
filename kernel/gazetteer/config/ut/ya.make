UNITTEST()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

PEERDIR(
    ADDINCL kernel/gazetteer/config
)

SRCDIR(kernel/gazetteer/config)

SRCS(
    protoconf_ut.cpp
    test.proto
)

END()
