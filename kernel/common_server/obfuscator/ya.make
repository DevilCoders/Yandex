LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/api/links
    kernel/common_server/util
    kernel/common_server/obfuscator/obfuscators
    kernel/common_server/library/kv
)

SRCS(
    config.cpp
    manager.cpp
    object.cpp
)

END()
