GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backupservice.go
    clusterservice.go
    databaseservice.go
    models.go
    perfdiagservice.go
    userservice.go
)

END()

RECURSE(gotest)
