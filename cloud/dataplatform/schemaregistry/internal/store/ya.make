GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    db_error.go
    store.go
)

END()

RECURSE(
    dbscan
    pgxscan
    postgres
)
