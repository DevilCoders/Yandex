LIBRARY()

OWNER(shuster)

PEERDIR(
    contrib/libs/protobuf
    kernel/indexer/direct_text
    kernel/indexer_iface
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/tarc/iface
    library/cpp/charset
    library/cpp/langmask
    library/cpp/langs
    library/cpp/string_utils/base64
    library/cpp/token
    library/cpp/wordpos
)

SRCS(
    directtext.cpp
    docinfo.h
    inserter.h
)

END()
