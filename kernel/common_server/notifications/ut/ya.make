UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/testing/unittest
    kernel/common_server/notifications
    kernel/common_server/library/async_proxy/ut/helper
    kernel/common_server/abstract
    kernel/common_server/library/json
    kernel/common_server/startrek
)

SIZE(SMALL)

SRCS(
    mail_ut.cpp
    push_ut.cpp
    sms_ut.cpp
    startrek_ut.cpp
)


END()
