LIBRARY()

OWNER(
    ivanmorozov
    g:geosaas
)

GENERATE_ENUM_SERIALIZATION(common.h)

SRCS(
    common.cpp
    environment.cpp
    guarded_ptr.h
    json.cpp
)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/json
    library/cpp/logger/global
)

END()
