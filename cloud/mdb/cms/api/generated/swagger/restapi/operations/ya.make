GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_cmsapi_api.go)

END()

RECURSE(
    monrun
    stability
    tasks
)
