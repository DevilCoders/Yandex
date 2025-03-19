GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    datetime.go
    duration.go
)

GO_XTEST_SRCS(
    datetime_test.go
    duration_test.go
)

END()

RECURSE(gotest)
