LIBRARY()

OWNER(
    leo
    g:base
)

SRCS(
    directindex.cpp
    directtarc.cpp
    directtokenizer.cpp
    extratext.cpp
)

PEERDIR(
    kernel/indexer/direct_text
    kernel/indexer/dtcreator
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/keyinv/indexfile
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/charset
    library/cpp/langmask
    library/cpp/langs
    library/cpp/token
    library/cpp/tokenizer
    library/cpp/wordpos
    ysite/directtext/textarchive
)

END()
