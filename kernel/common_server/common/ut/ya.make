UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/common
    kernel/common_server/notifications/abstract
    kernel/common_server/obfuscator/obfuscators
)

SRCS(
    validate_scheme_ut.cpp
)

END()
