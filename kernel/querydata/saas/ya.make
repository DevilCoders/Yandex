LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_saas_kv_key.cpp
    qd_saas_key_transform.cpp
    qd_saas_request.cpp
    qd_saas_response.cpp
)


PEERDIR(
    kernel/querydata/idl
    kernel/querydata/client
    kernel/querydata/common
    kernel/querydata/cgi
    kernel/querydata/request
    kernel/querydata/saas/idl
    kernel/querydata/saas/key
    kernel/querydata/server
    kernel/saas_trie/idl
    library/cpp/infected_masks
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

END()
