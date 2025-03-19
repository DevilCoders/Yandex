UNITTEST()

OWNER(
    g:base
    g:morphology
)

PEERDIR(
    ADDINCL kernel/lemmer/alpha
)

SRCDIR(kernel/lemmer/alpha)

SRCS(
    directory_ut.cpp
    normalizer_ut.cpp
)

END()
