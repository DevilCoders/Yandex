LIBRARY()

OWNER(g:snippets)

SRCS(
    querylog.cpp
)

PEERDIR(
    contrib/libs/re2
    library/cpp/deprecated/split
)

END()
