LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    enum_wrapper.h
    namespace.h
    stream_type.h
    dummy.cpp
)

GENERATE_ENUM_SERIALIZATION(namespace.h)

PEERDIR(
    library/cpp/wordpos
)

END()
