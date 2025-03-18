UNITTEST()

OWNER(zador)

PEERDIR(
    ADDINCL library/cpp/minhash
)

SRCDIR(library/cpp/minhash)

SRCS(
    minhash_ut.cpp
    prime_ut.cpp
    table_ut.cpp
)

END()
