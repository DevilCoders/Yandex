LIBRARY()

OWNER(
    g:blender
)

PEERDIR(
    kernel/blender/factor_storage/protos
    library/cpp/streams/lz
    library/cpp/string_utils/base64
)

SRCS(
    serialization.cpp
)

END()

RECURSE(
    protos
    pylib
)

RECURSE_FOR_TESTS(
    test
)
