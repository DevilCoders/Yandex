LIBRARY()

OWNER(
    agusakov
    plato
)

PEERDIR(
    contrib/libs/libidn
    library/cpp/charset
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    library/cpp/tokenizer
)

SRCS(
    analyze.cpp
    impl.cpp
    opts.cpp
    prepare.cpp
    public.cpp
    token.cpp
)

END()
