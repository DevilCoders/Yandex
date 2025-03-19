GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    converter.go
    errors.go
)

END()

RECURSE(
    impl
    mocks
)
