GO_LIBRARY()

OWNER(g:mdb)

SRCS(datatypes.go)

END()

RECURSE(
    app
    database
    dcs
    telemetry
)
