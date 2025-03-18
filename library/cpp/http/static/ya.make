LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/str_map
    library/cpp/http/misc
    library/cpp/http/server
)

SRCS(
    static.cpp
)

END()
