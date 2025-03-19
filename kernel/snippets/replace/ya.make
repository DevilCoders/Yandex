LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/config
    kernel/snippets/sent_match
    kernel/snippets/smartcut
    kernel/snippets/titles/make_title
    library/cpp/html/pcdata
)

SRCS(
    replace.cpp
    replaceresult.cpp
)

END()
