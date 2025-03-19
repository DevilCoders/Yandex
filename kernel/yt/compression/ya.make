OWNER (
    evseevd
    g:jupiter
)

LIBRARY()

SRCS (
    compression.cpp
)

PEERDIR (
    kernel/yt/attrs
    kernel/yt/utils
    mapreduce/yt/interface
)

GENERATE_ENUM_SERIALIZATION(compression.h)

END()
