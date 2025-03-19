GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app.go
    keysmanager.go
    minionpinger.go
)

GO_TEST_SRCS(
    keysmanager_test.go
    minionpinger_test.go
)

END()

RECURSE(
    gotest
    saltkeys
)
