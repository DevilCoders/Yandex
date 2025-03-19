GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    agent.go
    request_processor.go
    response_forwarder.go
)

END()

RECURSE(websockets)
