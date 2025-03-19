GO_PROGRAM(filestore-http-proxy)

OWNER(g:cloud-nbs)

SRCS(
    main.go
)

GO_TEST_SRCS(
    main_test.go
)

END()

RECURSE_FOR_TESTS(
    tests
    ut
)
