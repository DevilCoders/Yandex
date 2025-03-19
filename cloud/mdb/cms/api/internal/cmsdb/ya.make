GO_LIBRARY()

OWNER(g:mdb)

SRCS(cmsdb.go)

END()

RECURSE(
    mocks
    pg
)
