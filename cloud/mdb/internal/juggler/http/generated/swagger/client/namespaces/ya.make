GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    namespaces_client.go
    v2_namespaces_get_namespaces_parameters.go
    v2_namespaces_get_namespaces_responses.go
    v2_namespaces_remove_namespace_parameters.go
    v2_namespaces_remove_namespace_responses.go
    v2_namespaces_set_namespace_parameters.go
    v2_namespaces_set_namespace_responses.go
)

END()
