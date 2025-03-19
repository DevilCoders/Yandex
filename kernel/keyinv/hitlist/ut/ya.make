UNITTEST()

OWNER(leo)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
)

SRCDIR(kernel/keyinv/hitlist)

SRCS(
    hits_raw_ut.cpp
    positerator_ut.cpp
)

END()
