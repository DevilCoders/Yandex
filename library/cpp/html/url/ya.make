OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    contrib/libs/libidn
    library/cpp/charset
    library/cpp/uri
    library/cpp/html/entity
)

SRCS(
    url.cpp
)

END()
