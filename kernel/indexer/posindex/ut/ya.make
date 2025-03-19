OWNER(g:base)

UNITTEST()

PEERDIR(
    ADDINCL kernel/indexer/posindex
)

SRCDIR(kernel/indexer/posindex)

SRCS(
    invcreator_ut.cpp
)

END()
