GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    agent_handler.go
    server.go
)

END()

RECURSE(rewriters)
