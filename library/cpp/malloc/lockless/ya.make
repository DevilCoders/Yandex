LIBRARY()

NO_UTIL()

OWNER(nga)

PEERDIR(
    library/cpp/malloc/api
    contrib/libs/lockless
)

SRCS(
    malloc-info.cpp
)

END()
