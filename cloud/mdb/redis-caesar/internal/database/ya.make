GO_LIBRARY()

OWNER(g:mdb)

SRCS(database.go)

END()

RECURSE(
    mock
    redis
)
