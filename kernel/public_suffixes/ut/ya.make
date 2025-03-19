OWNER(g:cpp-contrib)

UNITTEST()

PEERDIR(
    ADDINCL kernel/public_suffixes
)

SRCDIR(kernel/public_suffixes)

SRCS(
    public_suffixes_ut.cpp
)

END()
