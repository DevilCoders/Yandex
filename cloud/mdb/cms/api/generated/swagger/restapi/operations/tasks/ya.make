GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    delete_task.go
    delete_task_parameters.go
    delete_task_responses.go
    delete_task_urlbuilder.go
    get_request_status.go
    get_request_status_parameters.go
    get_request_status_responses.go
    get_request_status_urlbuilder.go
    list_unhandled_management_requests.go
    list_unhandled_management_requests_parameters.go
    list_unhandled_management_requests_responses.go
    list_unhandled_management_requests_urlbuilder.go
    register_request_to_manage_nodes.go
    register_request_to_manage_nodes_parameters.go
    register_request_to_manage_nodes_responses.go
    register_request_to_manage_nodes_urlbuilder.go
)

END()
