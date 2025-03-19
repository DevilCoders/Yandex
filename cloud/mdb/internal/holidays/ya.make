GO_LIBRARY()

OWNER(g:mdb)

SRCS(holidays.go)

END()

RECURSE(
    httpapi
    internal
    memoized
    mocks
    weekends
    workaholic
)
