LIBRARY()
OWNER(kostik)

PEERDIR(
    contrib/libs/protobuf
)

SRCS(
    field_id_macro.cpp
    field_size_macro.cpp
    inplace.cpp
    parser.cpp
    region_data_provider.cpp
    serialize_sizes.cpp
    serialized.cpp
)

END()
