UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/tvm_services
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/library/tvm_services/abstract/request
    kernel/common_server/common
    kernel/common_server/obfuscator/obfuscators
)


SRCS(
    common_ut.cpp
    ct_parser_ut.cpp
)

TAG()

TIMEOUT(600)

SIZE(MEDIUM)

FORK_SUBTESTS()

SPLIT_FACTOR(10)

END()
