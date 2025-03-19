GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    macros.go
    porto.go
)

GO_TEST_SRCS(
    macros_test.go
    porto_test.go
)

END()

RECURSE(gotest)
