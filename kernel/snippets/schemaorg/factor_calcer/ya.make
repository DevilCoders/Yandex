LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/custom/schemaorg_viewer
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/span
    library/cpp/tokenizer
)

SRCS(
    schemaorg_factor_calcer.cpp
)

END()
