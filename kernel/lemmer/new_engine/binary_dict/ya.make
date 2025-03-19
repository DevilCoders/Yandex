LIBRARY()

OWNER(g:morphology)

SRCS(
    binary_dict.cpp
    binary_dict_header.cpp
)

PEERDIR(
    library/cpp/digest/crc32c
    kernel/lemmer/new_engine/iface
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(binary_dict_header.h)

END()
