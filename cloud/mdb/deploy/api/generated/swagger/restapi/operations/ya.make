GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_deployapi_api.go)

END()

RECURSE(
    commands
    common
    groups
    masters
    minions
)
