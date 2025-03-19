LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/replace
    kernel/snippets/sent_match
    util/charset
    library/cpp/unicode/normalization
)

SRCS(
    canonize_unicode.cpp
)

END()
