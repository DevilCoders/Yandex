GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    after_walle.go
    analyse_data_loss.go
    analyse_default.go
    base.go
    cleanup.go
    common.go
    let_go.go
    to_return.go
)

GO_XTEST_SRCS(base_test.go)

END()

RECURSE(gotest)
