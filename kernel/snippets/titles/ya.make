LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/alpha
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/archive/zone_checker
    kernel/snippets/config
    kernel/snippets/custom/forums_handler
    kernel/snippets/custom/opengraph
    kernel/snippets/custom/remove_emoji
    kernel/snippets/qtree
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/simple_textproc/capital
    kernel/snippets/smartcut
    kernel/snippets/titles/make_title
    kernel/snippets/urlcut
    kernel/snippets/urlmenu/dump
    kernel/snippets/wordstat
    kernel/web_factors_info
    library/cpp/stopwords
    library/cpp/string_utils/url
    library/cpp/unicode/punycode
)

SRCS(
    header_based.cpp
    titles_additions.cpp
    url_titles.cpp
)

END()
