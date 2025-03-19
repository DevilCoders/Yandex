LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/core
    kernel/snippets/archive/chooser
    kernel/snippets/archive/markup
    kernel/snippets/archive/unpacker
    kernel/snippets/archive/view
    kernel/snippets/config
    kernel/snippets/custom/forums_handler
    kernel/snippets/qtree
    kernel/snippets/telephone
    kernel/tarc/iface
    library/cpp/langs
)

SRCS(
    doc_lang.cpp
    metadata_viewer.cpp
    staticattr.cpp
    viewers.cpp
)

END()
