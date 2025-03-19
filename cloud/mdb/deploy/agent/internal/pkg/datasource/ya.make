GO_LIBRARY()

OWNER(g:mdb)

SRCS(datasource.go)

END()

RECURSE(
    mocks
    s3
)
