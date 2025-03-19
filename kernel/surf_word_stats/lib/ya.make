LIBRARY()

OWNER(smikler)

PEERDIR(
    library/cpp/compproto
    library/cpp/containers/comptrie
    library/cpp/streams/factory
)

SRCS(
    builder.cpp
    reader.cpp
)

GENERATE_ENUM_SERIALIZATION(common.h)

END()
