LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/dynamic_data
    kernel/snippets/factors
    kernel/snippets/iface/archive
    kernel/snippets/plm
    kernel/snippets/qtree
    kernel/snippets/read_helper
    kernel/snippets/schemaorg/factor_calcer
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/title_trigram
    kernel/snippets/titles/make_title
    kernel/snippets/wordstat
    kernel/web_factors_info
    library/cpp/string_utils/url
)

SRCS(
    weighter.cpp
)

END()
