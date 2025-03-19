OWNER(
    dmitryno
    gotmanov
    g:wizard
)

UNITTEST()

PEERDIR(
    ADDINCL kernel/gazetteer/common
)

SRCDIR(kernel/gazetteer/common)

SRCS(
    common_ut.cpp
    #pushinput_ut.cpp
)

END()
