LIBRARY()

OWNER(g:snippets)


PEERDIR(
    kernel/snippets/archive/markup
    kernel/snippets/archive/view
    kernel/snippets/iface/archive
    kernel/snippets/smartcut
    kernel/tarc/iface
    kernel/url_tools
    library/cpp/token
    library/cpp/tokenizer
)

SRCS(
    beautify.cpp
    sent_info.cpp
)

END()
