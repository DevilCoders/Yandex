LIBRARY()

NO_UTIL()

OWNER(pg g:util)

PEERDIR(
    library/cpp/malloc/api
    contrib/libs/mimalloc
)

SRCS(
    info.cpp
)

END()

RECURSE(
    link_test
)
