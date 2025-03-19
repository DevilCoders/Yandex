LIBRARY()

OWNER(g:saas)

SRCS(
    archive.cpp
    part.cpp
    part_header.cpp
    data_accessor.cpp
    optimization_options.cpp
    globals.cpp
)

GENERATE_ENUM_SERIALIZATION(data_accessor.h)
GENERATE_ENUM_SERIALIZATION(part.h)

PEERDIR(
    library/cpp/logger/global
)

END()
