UNITTEST()

OWNER(gotmanov)

PEERDIR(
    kernel/reqbundle
    library/cpp/string_utils/base64
)

SRCS(
    merge_ut.cpp
    remap_ut.cpp
    restrict_ut.cpp
    serialize_ut.cpp
    split_ut.cpp
    validate_ut.cpp
)

END()
