LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
    g:saas
)

SRCS(
    proto_txt_helper.cpp
    qd_raw_trie_conversion.cpp
    qd_saas_yt.cpp
)

PEERDIR(
    kernel/querydata/common
    kernel/querydata/idl
    kernel/querydata/indexer2
    kernel/querydata/saas
    kernel/querydata/saas_yt/idl
    kernel/querydata/server
    library/cpp/json
    library/cpp/string_utils/relaxed_escaper
    ysite/yandex/reqanalysis
    contrib/libs/protobuf
)

END()
