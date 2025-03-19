UNITTEST()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_cgi_ut.cpp
    qd_dump_ut.cpp
    qd_indexer_ut.cpp
    qi_parser_ut.cpp
    qd_request_ut.cpp
    qd_saas_ut.cpp
    qd_saas_yt_ut.cpp
    qd_scheme_ut.cpp
    qd_server_ut.cpp
    qd_trie_ut.cpp
    querydata_ut.cpp
)

PEERDIR(
    kernel/querydata/cgi
    kernel/querydata/client
    kernel/querydata/common
    kernel/querydata/dump
    kernel/querydata/idl
    kernel/querydata/indexer
    kernel/querydata/request
    kernel/querydata/saas
    kernel/querydata/saas_yt
    kernel/querydata/scheme
    kernel/querydata/server
    kernel/querydata/tries
    kernel/querydata/ut_utils
    library/cpp/archive
    library/cpp/infected_masks
    library/cpp/on_disk/codec_trie
    library/cpp/on_disk/coded_blob
    library/cpp/on_disk/meta_trie
    library/cpp/scheme
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
)

END()
