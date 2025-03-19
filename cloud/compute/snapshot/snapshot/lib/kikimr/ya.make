GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    commondb.go
    kikimr.go
    kikimr_chunks.go
    kikimr_create.go
    kikimr_delete.go
    kikimr_get.go
    kikimr_list.go
    kikimr_update.go
    kikimr_util.go
    kikimrdb.go
    tx_per_query.go
)

GO_TEST_SRCS(
    kikimr_test.go
    kikimr_util_test.go
    tx_per_query_test.go
)

END()

RECURSE(gotest)
