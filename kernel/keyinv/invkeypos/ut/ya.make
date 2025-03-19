UNITTEST()

OWNER(g:base)

PEERDIR(
    kernel/keyinv/invkeypos
)

SRCDIR(kernel/keyinv/invkeypos)

SRCS(
    doc_id_iterator_ut.cpp
    keyconv_ut.cpp
    keycode_ut.cpp
)

END()
