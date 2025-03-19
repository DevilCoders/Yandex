GO_PROGRAM()

OWNER(g:cloud-nbs)

SRCS(
    add.go
    kikimr.go
    main.go
    nbs.go
)

GO_TEST_SRCS(
    add_test.go
)

END()

RECURSE_FOR_TESTS(tests)
