LIBRARY()

OWNER(borzunov)

PEERDIR(
    kernel/lemmer
    library/cpp/lrucache
    library/cpp/numerator
)

SRCS(
    simple_simhash.h
    simple_simhash.cpp
)

END()
