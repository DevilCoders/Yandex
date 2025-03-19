LIBRARY()

OWNER(g:base)

SRCS(
    disamber.cpp
)

PEERDIR(
    dict/disamb/zel_disamb
    kernel/indexer/direct_text
    kernel/indexer/face
)

END()
