LIBRARY()

OWNER(
    stanly
    velavokr
)

SRCS(
    segment_span.cpp
    segment_span_decl.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/packedtypes
    library/cpp/string_utils/base64
    library/cpp/wordpos
)

END()

