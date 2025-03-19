LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    norm_query.cpp
    norm_structkey.cpp
    norm_util.cpp
    qd_inferiority.cpp
    qd_genrequests.cpp
    qd_key_patcher.cpp
    qd_rawkeys.cpp
    qd_subkeys_util.cpp
)

PEERDIR(
    kernel/hosts/owner
    kernel/searchlog
    kernel/querydata/cgi
    kernel/querydata/common
    kernel/querydata/idl
    kernel/querydata/idl/scheme
    kernel/urlid
    library/cpp/scheme
    util/draft
    ysite/yandex/doppelgangers
    ysite/yandex/reqanalysis
)

END()
