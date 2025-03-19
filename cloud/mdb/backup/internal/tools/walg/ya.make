GO_LIBRARY()

OWNER(g:mdb)

SRCS(wal-g.go)

END()

RECURSE(
    mysql
    postgresql
)
