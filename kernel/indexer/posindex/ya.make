LIBRARY()

OWNER(g:base)

SRCS(
    posindex.cpp
    invcreator.cpp
    invserial.cpp
    directins.cpp
    memindex.h
    faceint.h
    docdata.h
)

PEERDIR(
    kernel/indexer/direct_text
    kernel/indexer/disamber
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/indexer_iface
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/charset
    library/cpp/containers/mh_heap
    library/cpp/containers/str_hash
    library/cpp/token
    library/cpp/wordpos
    ysite/yandex/common
)

END()
