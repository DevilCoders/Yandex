LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/pure_lib
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/iface/archive
    library/cpp/scheme
    library/cpp/stopwords
)

SRCS(
    automatos.cpp
    entitycl.cpp
)

END()
