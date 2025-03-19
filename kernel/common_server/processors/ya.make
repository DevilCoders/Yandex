LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/processors/db_entity
    kernel/common_server/processors/db_migrations
    kernel/common_server/processors/emulation
    kernel/common_server/processors/http_proxy
    kernel/common_server/processors/notifiers
    kernel/common_server/processors/proposition
    kernel/common_server/processors/roles
    kernel/common_server/processors/rt_background
    kernel/common_server/processors/settings
    kernel/common_server/processors/snapshot
    kernel/common_server/processors/system
    kernel/common_server/processors/tags
    kernel/common_server/processors/user_auth
    kernel/common_server/processors/user_role
    kernel/common_server/processors/obfuscator
    kernel/common_server/processors/forward_proxy
    kernel/common_server/processors/miracle
)

END()
