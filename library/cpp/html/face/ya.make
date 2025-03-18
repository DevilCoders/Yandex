LIBRARY()

OWNER(stanly)

SRCS(
    event.cpp
    noindex.rl6
    onchunk.h
    out.cpp
    parsface.h
    parstypes.h
    propface.proto
    propface.cpp
    zonecount.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/str_hash
    library/cpp/html/entity
    library/cpp/html/spec
    library/cpp/mime/types
    library/cpp/uri
)

GENERATE_ENUM_SERIALIZATION(parstypes.h)

END()
