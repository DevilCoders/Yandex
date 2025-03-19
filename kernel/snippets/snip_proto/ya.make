LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/algo
    kernel/snippets/calc_dssm
    kernel/snippets/config
    kernel/snippets/cut
    kernel/snippets/custom/schemaorg_viewer
    kernel/snippets/explain
    kernel/snippets/factors
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/titles/make_title
    kernel/snippets/uni_span_iter
    kernel/snippets/weight
    kernel/snippets/wordstat
)

SRCS(
    snip_proto_2.cpp
)

END()
