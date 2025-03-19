UNITTEST()

OWNER(
    g:base
)

PEERDIR(
    ADDINCL kernel/bundle
    kernel/matrixnet
)

SRCDIR(kernel/bundle)

SRCS(
    bundle_ut.cpp
)

END()
