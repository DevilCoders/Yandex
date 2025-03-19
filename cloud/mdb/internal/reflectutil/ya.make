GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    copy_struct_fields.go
    merge_structs.go
    opts.go
    reverse.go
)

GO_XTEST_SRCS(
    copy_struct_fields_test.go
    merge_structs_test.go
    reverse_test.go
)

END()

RECURSE(
    converters
    gotest
)
