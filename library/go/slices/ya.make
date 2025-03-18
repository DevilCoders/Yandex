GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    contains.go
    contains_all.go
    contains_any.go
    dedup.go
    equal.go
    filter.go
    intersects.go
    join.go
    map.go
    reverse.go
    shuffle.go
)

GO_XTEST_SRCS(
    dedup_test.go
    equal_test.go
    filter_test.go
    intersects_test.go
    join_test.go
    map_test.go
    reverse_test.go
    shuffle_test.go
)

END()

RECURSE(gotest)
