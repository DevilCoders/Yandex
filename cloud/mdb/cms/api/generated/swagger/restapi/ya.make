GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    configure_mdb_cmsapi.go
    doc.go
    embedded_spec.go
    server.go
)

END()

RECURSE(operations)
