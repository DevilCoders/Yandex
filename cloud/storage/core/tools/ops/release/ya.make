GO_PROGRAM()

OWNER(g:cloud-nbs)

SRCS(
    main.go
)

GO_EMBED_PATTERN(telegram.users)
GO_EMBED_PATTERN(qmssngr.users)

END()

RECURSE_FOR_TESTS(gotest)
