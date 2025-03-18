OWNER(g:personal-search)

LIBRARY()

SRCS(
    hx_sanitize.cpp
    hx_attrs.rl6
)

PEERDIR(
    library/cpp/charset
    library/cpp/html/entity
    library/cpp/html/lexer
    library/cpp/html/sanitize/css
    library/cpp/html/spec
    library/cpp/uri
)

END()
