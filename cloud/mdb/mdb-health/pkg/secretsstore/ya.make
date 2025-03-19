GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    doc.go
    secretsstore.go
)

END()

RECURSE(
    mocks
    pg
)
