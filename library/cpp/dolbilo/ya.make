LIBRARY()

OWNER(
    pg
    darkk
    mvel
)

SRCS(
    plan.cpp
    stat.cpp
    chunk.cpp
    execmode.cpp
)

GENERATE_ENUM_SERIALIZATION(execmode.h)

PEERDIR(
    library/cpp/uri
    library/cpp/http/misc
    library/cpp/http/io
)

END()
