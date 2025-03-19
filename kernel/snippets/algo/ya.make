LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/factors
    kernel/snippets/hits
    kernel/snippets/iface/archive
    kernel/snippets/qtree
    kernel/snippets/sent_info
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/snip_builder
    kernel/snippets/titles/make_title
    kernel/snippets/uni_span_iter
    kernel/snippets/video
    kernel/snippets/weight
    kernel/snippets/wordstat
    library/cpp/langs
    library/cpp/stopwords
)

SRCS(
    extend.cpp
    maxfit.cpp
    one_span.cpp
    redump.cpp
    two_span.cpp
    video_span.cpp
)

END()
