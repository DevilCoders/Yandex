LIBRARY()

OWNER(aydar)

SRCS(
    wrappers.cpp
)

PEERDIR(
    contrib/libs/mongo-c-driver/libmongoc
    contrib/libs/mongo-c-driver/libbson
    library/cpp/json
)

END()
