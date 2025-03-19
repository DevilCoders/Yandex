GO_LIBRARY()

OWNER(g:mdb)

SRCS(invoicer.go)

GO_TEST_SRCS(invoicer_test.go)

END()

RECURSE(
    cloudstorage
    gotest
    simplebackup
)
