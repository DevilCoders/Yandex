OWNER(
    iddqd
    ivanmorozov
)

LIBRARY()

SRCS(
    idstorage.cpp
)

PEERDIR(
    kernel/indexer/attrproc
    kernel/indexer/baseproc
    kernel/indexer/direct_text
    kernel/indexer/directindex
    kernel/indexer/disamber
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/indexer/parseddoc
    kernel/indexer/posindex
    kernel/indexer/tfproc
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/queryfactors
    kernel/search_zone
    kernel/titlefeatures
    library/cpp/html/storage
    ysite/directtext/dater
    ysite/directtext/freqs
    ysite/directtext/sentences_lens
    ysite/directtext/textarchive
    ysite/yandex/common
    ysite/yandex/pure
    yweb/protos
)

END()
