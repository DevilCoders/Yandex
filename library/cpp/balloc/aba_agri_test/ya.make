UNITTEST()

OWNER(
    agri
    mvel
)

ALLOCATOR(B)

SRCS(
    balloc_aba_ut.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

TAG(ya:not_autocheck)

END()

NEED_CHECK()
