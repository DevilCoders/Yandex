GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dns.go
    errors.go
    external_resources.go
    gateway.go
    helpers.go
    network_connection_create.go
    network_connection_delete.go
    network_connection_params.go
    network_create.go
    network_delete.go
    peering.go
    ram.go
    routes.go
    security_group.go
    service.go
    subnet.go
    tags.go
    transit_gateway.go
    vpc.go
    vpc_import.go
    zone.go
)

GO_XTEST_SRCS(
    dns_test.go
    gateway_test.go
    network_connection_create_test.go
    ram_test.go
    security_group_test.go
    subnet_test.go
    tags_test.go
    vpc_test.go
)

END()

RECURSE(
    gotest
    ready
)
