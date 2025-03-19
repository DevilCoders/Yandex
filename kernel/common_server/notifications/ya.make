LIBRARY()

OWNER(g:cs_dev)

SRCS(
    manager.cpp
)

PEERDIR(
    kernel/common_server/notifications/abstract
    kernel/common_server/notifications/internal
    kernel/common_server/notifications/logs
    kernel/common_server/notifications/mail
    kernel/common_server/notifications/null
    kernel/common_server/notifications/push
    kernel/common_server/notifications/sms
    kernel/common_server/notifications/startrek
    kernel/common_server/notifications/telegram
    kernel/common_server/notifications/ucommunications
)

END()
