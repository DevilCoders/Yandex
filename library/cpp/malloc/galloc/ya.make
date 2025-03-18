LIBRARY()

NO_UTIL()

OWNER(nga)

PEERDIR(
    library/cpp/malloc/api
    contrib/deprecated/galloc
)

SRCS(
    malloc-info.cpp
)

END()
