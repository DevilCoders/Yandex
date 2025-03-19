LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_trie.cpp
)

PEERDIR(
    kernel/doc_url_index
    kernel/querydata/common
    kernel/querydata/idl
    library/cpp/containers/comptrie
    library/cpp/on_disk/codec_trie
    library/cpp/on_disk/coded_blob
    library/cpp/on_disk/meta_trie
    library/cpp/deprecated/solartrie
    library/cpp/infected_masks
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

END()
