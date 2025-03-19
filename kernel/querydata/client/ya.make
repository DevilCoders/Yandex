LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_client_utils.cpp
    qd_key.cpp
    qd_merge.cpp
    qd_document_responses.cpp
    querydata.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/urlid
    kernel/querydata/common
    kernel/querydata/idl
    library/cpp/scheme
    util/draft
    library/cpp/string_utils/url
)

END()
