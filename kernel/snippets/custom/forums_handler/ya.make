LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/snippets/archive/chooser
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/archive/zone_checker
    library/cpp/tokenizer
)

SRCS(
    forums_handler.cpp
)

END()
