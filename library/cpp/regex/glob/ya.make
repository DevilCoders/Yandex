LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    glob.cpp
    glob_iterator.cpp
)

PEERDIR(
    library/cpp/charset
)

END()
