GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

SRCS(models.go)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(
    clickhouse
    console
    gotest
    kafka
)
