LIBRARY()

OWNER(divankov)

SRCS(
    common.cpp
    fio.cpp
    nohtml.rl6
)

PEERDIR(
    library/cpp/html/dehtml
    kernel/qtree/richrequest
)

END()
