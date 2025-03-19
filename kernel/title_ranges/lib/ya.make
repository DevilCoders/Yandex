LIBRARY()

OWNER(gotmanov)

PEERDIR(
    library/cpp/bit_io
    library/cpp/charset
    library/cpp/resource
)

SRCS(
    doc_title_range.h
    title_ranges.cpp
)

GENERATE_ENUM_SERIALIZATION(doc_title_range.h)

END()
