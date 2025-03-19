OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    contrib/libs/libidn
    kernel/url
)

SRCS(
    public_suffixes.h
    public_suffixes.cpp
)

END()
