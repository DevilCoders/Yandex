LIBRARY()

OWNER(g:indexann)

SRCS(
    writer.cpp
)

PEERDIR(
    dict/recognize/queryrec
    kernel/indexann/interface
    kernel/indexann/protos
    kernel/indexer/directindex
    kernel/indexer/posindex
    kernel/keyinv/indexfile
    kernel/sent_lens
    kernel/tarc/iface
    kernel/walrus
    util/draft
    ysite/directtext/freqs
    ysite/directtext/sentences_lens
    ysite/yandex/pure
)

END()
