OWNER(ilnurkh)

LIBRARY()

SRCS(
    features_remap.cpp
)

PEERDIR(
    kernel/features_remap/metadata
    kernel/factor_slices
    library/cpp/string_utils/quote
    library/cpp/string_utils/base64
    library/cpp/protobuf/util
    library/cpp/expression
)

END()


RECURSE_FOR_TESTS(ut)

