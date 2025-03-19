GO_LIBRARY()

OWNER(
    g:data-transfer
    v01d
    tserakhau
)

SRCS(
    context.go
    storage.go
)

END()

RECURSE(types)
