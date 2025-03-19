GO_LIBRARY()

OWNER(g:mdb)

SRCS(deploy.go)

END()

RECURSE(
    commands
    groups
    helpers
    jobresults
    masters
    minions
    paging
    shipments
)
