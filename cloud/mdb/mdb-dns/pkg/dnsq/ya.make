GO_LIBRARY()

OWNER(g:mdb)

SRCS(dnsq.go)

END()

RECURSE(
    compute
    dnsal
    mem
    route53
)
