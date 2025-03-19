GO_LIBRARY()

OWNER(
    g:data-transfer
    tserakhau
)

RESOURCE(
    transfer_manager/go/internal/swag/swagger.json swagger.json
)

SRCS(
    mixer.go
    swag.go
)

END()
