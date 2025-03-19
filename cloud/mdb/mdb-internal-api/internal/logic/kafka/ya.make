GO_LIBRARY()

OWNER(g:mdb)

SRCS(kafka.go)

END()

RECURSE(
    defaults
    helpers
    kfmodels
    mocks
    provider
    validation
)
