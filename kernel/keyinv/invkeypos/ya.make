LIBRARY()

OWNER(
    g:base
    mvel
)

SRCS(
    keychars.h
    keycode.cpp
    keyconv.cpp
    doc_id_iterator.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/unicode/normalization
    library/cpp/wordpos
    library/cpp/langs
    kernel/search_types
)

END()
