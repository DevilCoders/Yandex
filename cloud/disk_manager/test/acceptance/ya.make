OWNER(g:cloud-nbs)

GO_PROGRAM(acceptance-test)

SRCS(
    main.go
    ycp.go
)

END()

RECURSE(
    test_runner
)
