LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/querydata/cgi
    kernel/querydata/common
    kernel/querydata/idl
    kernel/querydata/request
    kernel/querydata/scheme
    kernel/querydata/server
    kernel/querydata/tries
    library/cpp/json
    library/cpp/scheme
    library/cpp/scheme/ut_utils
)

SRCS(
    qd_offline.cpp
    qd_ut_utils.cpp
)

END()
