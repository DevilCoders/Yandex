LIBRARY()

OWNER(g:jupiter)

RUN_PROGRAM(
    kernel/sent_lens/generate-trie
    STDOUT ${BINDIR}/sent_lens_data_trie_blob.inc
)

ARCHIVE(
    NAME trie_blob.inc
    ${BINDIR}/sent_lens_data_trie_blob.inc
)

PEERDIR(
    kernel/sent_lens/data
    kernel/doom/offroad_sent_wad
    kernel/doom/doc_lump_fetcher
    kernel/doom/search_fetcher
    library/cpp/offroad/codec
    library/cpp/archive
    library/cpp/on_disk/chunks
    library/cpp/sse
    library/cpp/packers
)

SRCS(
    sent_lens.cpp
    sent_lens_writer.cpp
    sent_lens_data_trie.cpp
)

END()
