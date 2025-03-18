UNITTEST()

OWNER(g:images-robot)

PEERDIR(
    ADDINCL library/cpp/consistent_hash_ring
)

SRCDIR(library/cpp/consistent_hash_ring)

SRCS(
    consistent_hash_ring_ut.cpp
)

END()
