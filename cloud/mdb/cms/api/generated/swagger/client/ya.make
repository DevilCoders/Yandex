GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_cmsapi_client.go)

END()

RECURSE(
    monrun
    stability
    tasks
)
