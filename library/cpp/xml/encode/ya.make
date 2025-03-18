LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    encodexml.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/html/pcdata
)

END()
