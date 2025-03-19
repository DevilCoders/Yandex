LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/auth/apikey
    kernel/common_server/auth/apikey2
    kernel/common_server/auth/blackbox
    kernel/common_server/auth/blackbox2
    kernel/common_server/auth/common
    kernel/common_server/auth/fake
    kernel/common_server/auth/meta
    kernel/common_server/auth/tvm
    kernel/common_server/auth/yang
    kernel/common_server/auth/jwt
)

END()
