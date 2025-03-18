LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/libs/libxml
    library/cpp/xml/init
)

SRCS(
    sax.cpp
    simple.cpp
)

END()
