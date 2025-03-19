GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusterservice.go
    models.go
    operationservice.go
)

END()

RECURSE(gotest)
