GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    entity.go
    entity_easyjson.go
    storage.go
    types.go
    types_db_methods.go
    types_easyjson.go
)

END()

RECURSE(
    postgres
    postgres_test
)
