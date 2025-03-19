GO_LIBRARY()

OWNER(g:mdb)

SRCS(cloudcrt_client.go)

END()

RECURSE(
    acl
    c_r_l
    ping
    s_s_l_certificates
    soft_certificates
)
