LIBRARY()

OWNER(
    g:base
    g:middle
    g:upper
)

SRCS(
    consts.cpp
    cutter.cpp
    cutter_menu.cpp
    decode_url.cpp
    hilited_string.cpp
    preprocess.cpp
    tokenizer.cpp
    tokens.cpp
    url_wanderer.cpp
    urlcut.cpp
)

PEERDIR(
    kernel/lemmer
    kernel/lemmer/untranslit
    kernel/qtree/richrequest
    kernel/snippets/simple_textproc/deyo
    kernel/snippets/custom/hostnames_data
    library/cpp/charset
    library/cpp/deprecated/dater_old/scanner
    library/cpp/langs
    library/cpp/on_disk/aho_corasick
    library/cpp/string_utils/quote
    library/cpp/tokenizer
    library/cpp/unicode/punycode
)

END()
