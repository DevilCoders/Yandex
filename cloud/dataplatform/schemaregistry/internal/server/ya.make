GO_LIBRARY()

OWNER(tserakhau)

SRCS(server.go)

END()

RECURSE(
    api
    domain
    namespace
    schema
    search
    validator
)
