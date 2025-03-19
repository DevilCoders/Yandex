LIBRARY()

OWNER(
    lagrunge
    g:base
)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/sse
    kernel/factors_info
    kernel/factor_slices
    library/cpp/codecs
)

SRCS(
    factors_reader.cpp
    factor_filler.cpp
    factor_selector.cpp
    factor_storage.cpp
    factor_view.cpp
    float_utils.cpp
)

END()
