OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    consts.go
    header.go
    reader.go
)

END()

RECURSE_FOR_TESTS(
    tests
)
